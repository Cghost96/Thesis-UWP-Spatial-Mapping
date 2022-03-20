//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"

#include <ppltasks.h>

#include <thread>

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "Common\DirectXHelper.h"
#include "Common\StepTimer.h"
#include "Common\Helper.h"
#include "GetDataFromIBuffer.h"
#include "SurfaceMesh.h"

using namespace SpatialMapping;

using namespace DX;
using namespace DirectX;
using namespace PackedVector;
using namespace Windows::Perception::Spatial;
using namespace Windows::Perception::Spatial::Surfaces;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::DirectX;
using namespace Platform;


SurfaceMesh::SurfaceMesh()
{
	std::lock_guard<std::mutex> lock(m_meshResourcesMutex);

	ReleaseDeviceDependentResources();
	m_lastUpdateTime.UniversalTime = 0;
}

SurfaceMesh::~SurfaceMesh()
{
	std::lock_guard<std::mutex> lock(m_meshResourcesMutex);

	ReleaseDeviceDependentResources();
}

void SurfaceMesh::UpdateSurface(
	SpatialSurfaceMesh^ surfaceMesh)
{
	m_pendingSurfaceMesh = surfaceMesh;
}

// Spatial Mapping surface meshes each have a transform. This transform is updated every frame.
void SurfaceMesh::UpdateTransform(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	DX::StepTimer const& timer,
	SpatialCoordinateSystem^ baseCoordinateSystem)
{
	UpdateVertexResources(device, baseCoordinateSystem);

	{
		std::lock_guard<std::mutex> lock(m_meshResourcesMutex);

		if (m_updateReady)
		{
			// Surface mesh resources are created off-thread so that they don't affect rendering latency.
			// When a new update is ready, we should begin using the updated vertex position, normal, and 
			// index buffers.
			SwapVertexBuffers();
			m_updateReady = false;
		}
	}

	XMMATRIX transform;
	if (m_isActive)
	{
		// In this example, new surfaces are treated differently by highlighting them in a different
		// color. This allows you to observe changes in the spatial map that are due to new meshes,
		// as opposed to mesh updates.
		if (m_colorFadeTimeout > 0.f)
		{
			m_colorFadeTimer += static_cast<float>(timer.GetElapsedSeconds());
			if (m_colorFadeTimer < m_colorFadeTimeout)
			{
				float colorFadeFactor = min(1.f, m_colorFadeTimeout - m_colorFadeTimer);
				m_constantBufferData.colorFadeFactor = XMFLOAT4(colorFadeFactor, colorFadeFactor, colorFadeFactor, 1.f);
			}
			else
			{
				m_constantBufferData.colorFadeFactor = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
				m_colorFadeTimer = m_colorFadeTimeout = -1.f;
			}
		}

		// The transform is updated relative to a SpatialCoordinateSystem. In the SurfaceMesh class, we
		// expect to be given the same SpatialCoordinateSystem that will be used to generate view
		// matrices, because this class uses the surface mesh for rendering.
		// Other applications could potentially involve using a SpatialCoordinateSystem from a stationary
		// reference frame that is being used for physics simulation, etc.
		auto tryTransform = m_meshProperties.localCoordSystem ? m_meshProperties.localCoordSystem->TryGetTransformTo(baseCoordinateSystem) : nullptr;
		if (tryTransform != nullptr)
		{
			// If the transform can be acquired, this spatial mesh is valid right now and
			// we have the information we need to draw it this frame.
			transform = XMLoadFloat4x4(&tryTransform->Value);
			m_lastActiveTime = static_cast<float>(timer.GetTotalSeconds());
		}
		else
		{
			// If the transform cannot be acquired, the spatial mesh is not valid right now
			// because its location cannot be correlated to the current space.
			m_isActive = false;
		}
	}

	if (!m_isActive)
	{
		// If for any reason the surface mesh is not active this frame - whether because
		// it was not included in the observer's collection, or because its transform was
		// not located - we don't have the information we need to update it.
		return;
	}

	// Set up a transform from surface mesh space, to world space.
	XMMATRIX scaleTransform = XMMatrixScalingFromVector(XMLoadFloat3(&m_meshProperties.vertexPositionScale));
	XMStoreFloat4x4(
		&m_constantBufferData.modelToWorld,
		XMMatrixTranspose(scaleTransform * transform)
	);

	// Surface meshes come with normals, which are also transformed from surface mesh space, to world space.
	XMMATRIX normalTransform = transform;
	// Normals are not translated, so we remove the translation component here.
	normalTransform.r[3] = XMVectorSet(0.f, 0.f, 0.f, XMVectorGetW(normalTransform.r[3]));
	XMStoreFloat4x4(
		&m_constantBufferData.normalToWorld,
		XMMatrixTranspose(normalTransform)
	);

	if (!m_constantBufferCreated)
	{
		// If loading is not yet complete, we cannot actually update the graphics resources.
		// This return is intentionally placed after the surface mesh updates so that this
		// code may be copied and re-used for CPU-based processing of surface data.
		CreateDeviceDependentResources(device);
		return;
	}

	context->UpdateSubresource(
		m_modelTransformBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
	);
}

// Does an indexed, instanced draw call after setting the IA stage to use the mesh's geometry, and
// after setting up the constant buffer for the surface mesh.
// The caller is responsible for the rest of the shader pipeline.
void SurfaceMesh::Draw(
	ID3D11Device* device,
	ID3D11DeviceContext* context,
	bool usingVprtShaders,
	bool isStereo)
{
	if (!m_constantBufferCreated || !m_loadingComplete)
	{
		// Resources are still being initialized.
		return;
	}

	if (!m_isActive)
	{
		// Mesh is not active this frame, and should not be drawn.
		return;
	}

	// The vertices are provided in {vertex, normal} format

	UINT strides[] = { m_meshProperties.vertexStride, m_meshProperties.normalStride };
	UINT offsets[] = { 0, 0 };
	ID3D11Buffer* buffers[] = { GetVertexPositions().Get(), GetVertexNormals().Get() };

	context->IASetVertexBuffers(
		0,
		ARRAYSIZE(buffers),
		buffers,
		strides,
		offsets
	);

	context->IASetIndexBuffer(
		GetTriangleIndices().Get(),
		m_meshProperties.indexFormat,
		0
	);

	context->VSSetConstantBuffers(
		0,
		1,
		m_modelTransformBuffer.GetAddressOf()
	);

	if (!usingVprtShaders)
	{
		context->GSSetConstantBuffers(
			0,
			1,
			m_modelTransformBuffer.GetAddressOf()
		);
	}

	context->PSSetConstantBuffers(
		0,
		1,
		m_modelTransformBuffer.GetAddressOf()
	);

	context->DrawIndexedInstanced(
		m_meshProperties.indexCount,       // Index count per instance.
		isStereo ? 2 : 1,   // Instance count.
		0,                  // Start index location.
		0,                  // Base vertex location.
		0                   // Start instance location.
	);
}

void SurfaceMesh::CreateDirectXBuffer(
	ID3D11Device* device,
	D3D11_BIND_FLAG binding,
	IBuffer^ buffer,
	ID3D11Buffer** target)
{
	CD3D11_BUFFER_DESC bufferDescription(buffer->Length, binding);
	D3D11_SUBRESOURCE_DATA bufferBytes = { GetDataFromIBuffer(buffer), 0, 0 };
	device->CreateBuffer(&bufferDescription, &bufferBytes, target);
}

void SurfaceMesh::UpdateVertexResources(
	ID3D11Device* device, Windows::Perception::Spatial::SpatialCoordinateSystem^ worldCoordSystem = nullptr)
{
	if (!m_isExpired && !m_isShuttingDown && worldCoordSystem != nullptr) {

		SpatialSurfaceMesh^ surfaceMesh = std::move(m_pendingSurfaceMesh);
		if (!surfaceMesh || surfaceMesh->TriangleIndices->ElementCount < 3)
		{
			// Not enough indices to draw a triangle or there is no pending mesh.
			return;
		}

		// Surface mesh resources are created off-thread, so that they don't affect rendering latency.
		m_updateVertexResourcesTask.then([this, device, surfaceMesh, worldCoordSystem]()
			{
				// Create new Direct3D device resources for the updated buffers. These will be set aside
				// for now, and then swapped into the active slot next time the render loop is ready to draw.
				std::lock_guard<std::mutex> lock(m_meshResourcesMutex);

				IBuffer^ positions = surfaceMesh->VertexPositions->Data;
				IBuffer^ normals = surfaceMesh->VertexNormals->Data;
				IBuffer^ indices = surfaceMesh->TriangleIndices->Data;

				SpatialCoordinateSystem^ const meshCoordSys = surfaceMesh->CoordinateSystem;
				IBox<float4x4>^ const meshCoordSysToWorld = meshCoordSys->TryGetTransformTo(worldCoordSystem);
				IBox<float4x4>^ const worldCoordSysToMesh = worldCoordSystem->TryGetTransformTo(meshCoordSys);

				if (meshCoordSysToWorld && worldCoordSysToMesh) {
					XMSHORTN4* const positionData = GetDataFromIBuffer<XMSHORTN4>(positions);
					XMBYTEN4* const normalData = GetDataFromIBuffer<XMBYTEN4>(normals);
					IndexFormat* const indexData = GetDataFromIBuffer<IndexFormat>(indices);

					if (positionData != nullptr && normalData != nullptr && indexData != nullptr) {
						m_id = surfaceMesh->GetHashCode();

						m_exportPositions.clear();
						float3 const pScale = surfaceMesh->VertexPositionScale;

						for (int i = 0; i < surfaceMesh->VertexPositions->ElementCount; i++)
						{
							// Extract from device
							XMFLOAT4 p;
							XMVECTOR const vec = XMLoadShortN4(&positionData[i]);
							XMStoreFloat4(&p, vec);

							// Scale/transform
							float3 const pScaled = float3(p.x * pScale.x, p.y * pScale.y, p.z * pScale.z);
							float3 pMeshToWorld = transform(pScaled, meshCoordSysToWorld->Value);

							// Cache for export
							m_exportPositions.push_back(pMeshToWorld);

							// Insert back into app
							//float3 const pWorldToMesh = transform(pMeshToWorld, worldCoordSysToMesh->Value);
							//p = { pWorldToMesh.x / pScale.x, pWorldToMesh.y / pScale.y, pWorldToMesh.z / pScale.z, p.w };

							//XMSHORTN4 pUpdated;
							//XMVECTOR const vecUpdated = XMLoadFloat4(&p);
							//XMStoreShortN4(&pUpdated, vecUpdated);

							//positionData[i] = pUpdated;
						}

						m_exportNormals.clear();

						for (int i = 0; i < surfaceMesh->VertexNormals->ElementCount; i++) {
							XMFLOAT4 n;
							XMVECTOR const vec = XMLoadByteN4(&normalData[i]);
							XMStoreFloat4(&n, vec);
							
							m_exportNormals.emplace_back(n.x, n.y, n.z);
						}

						m_exportIndices.clear();

						for (int i = 0; i < surfaceMesh->TriangleIndices->ElementCount; i++)
						{
							// +1 to get .obj format
							m_exportIndices.emplace_back(indexData[i] + 1);
						}
					}
				}

				// Then, we create Direct3D device buffers with the mesh data provided by HoloLens.
				Microsoft::WRL::ComPtr<ID3D11Buffer> updatedVertexPositions;
				Microsoft::WRL::ComPtr<ID3D11Buffer> updatedVertexNormals;
				Microsoft::WRL::ComPtr<ID3D11Buffer> updatedTriangleIndices;

				CreateDirectXBuffer(device, D3D11_BIND_VERTEX_BUFFER, positions, updatedVertexPositions.GetAddressOf());
				CreateDirectXBuffer(device, D3D11_BIND_VERTEX_BUFFER, normals, updatedVertexNormals.GetAddressOf());
				CreateDirectXBuffer(device, D3D11_BIND_INDEX_BUFFER, indices, updatedTriangleIndices.GetAddressOf());

				// Before updating the meshes, check to ensure that there wasn't a more recent update.
				auto meshUpdateTime = surfaceMesh->SurfaceInfo->UpdateTime;
				if (meshUpdateTime.UniversalTime > m_lastUpdateTime.UniversalTime)
				{
					// Prepare to swap in the new meshes.
					// Here, we use ComPtr.Swap() to avoid unnecessary overhead from ref counting.
					m_updatedVertexPositions.Swap(updatedVertexPositions);
					m_updatedVertexNormals.Swap(updatedVertexNormals);
					m_updatedTriangleIndices.Swap(updatedTriangleIndices);

					// Cache properties
					m_updatedMeshProperties.localCoordSystem = surfaceMesh->CoordinateSystem;
					m_updatedMeshProperties.vertexPositionScale = surfaceMesh->VertexPositionScale;
					m_updatedMeshProperties.vertexStride = surfaceMesh->VertexPositions->Stride;
					m_updatedMeshProperties.normalStride = surfaceMesh->VertexNormals->Stride;
					m_updatedMeshProperties.indexCount = surfaceMesh->TriangleIndices->ElementCount;
					m_updatedMeshProperties.indexFormat = static_cast<DXGI_FORMAT>(surfaceMesh->TriangleIndices->Format);

					// Send a signal to the render loop indicating that new resources are available to use.
					m_updateReady = true;
					m_lastUpdateTime = meshUpdateTime;
					m_loadingComplete = true;
				}
			});
	}
}

void SurfaceMesh::CreateDeviceDependentResources(
	ID3D11Device* device)
{
	UpdateVertexResources(device);

	// Create a constant buffer to control mesh position.
	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelNormalConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			&m_modelTransformBuffer
		)
	);

	m_constantBufferCreated = true;
}

void SurfaceMesh::ReleaseVertexResources()
{
	if (m_surfaceMesh)
	{
		m_pendingSurfaceMesh = m_surfaceMesh;
		m_surfaceMesh = nullptr;
	}

	m_meshProperties = {};
	GetVertexPositions().Reset();
	GetVertexNormals().Reset();
	GetTriangleIndices().Reset();
}

void SurfaceMesh::SwapVertexBuffers()
{
	// Swap out the previous vertex position, normal, and index buffers, and replace
	// them with up-to-date buffers.
	m_vertexPositions = m_updatedVertexPositions;
	m_vertexNormals = m_updatedVertexNormals;
	m_triangleIndices = m_updatedTriangleIndices;

	// Swap out the metadata: index count, index format, .
	m_meshProperties = m_updatedMeshProperties;

	m_updatedMeshProperties = {};
	m_updatedVertexPositions.Reset();
	m_updatedVertexNormals.Reset();
	m_updatedTriangleIndices.Reset();
}

void SurfaceMesh::ReleaseDeviceDependentResources()
{
	// Wait for pending vertex creation work to complete.
	m_updateVertexResourcesTask.wait();

	// Clear out any pending resources.
	SwapVertexBuffers();

	// Clear out active resources.
	ReleaseVertexResources();

	m_exportPositions.clear();
	m_exportIndices.clear();

	m_modelTransformBuffer.Reset();

	m_constantBufferCreated = false;
	m_loadingComplete = false;
}

void SurfaceMesh::Reset()
{
	std::lock_guard<std::mutex> lock(m_meshResourcesMutex);

	ReleaseDeviceDependentResources();
	m_lastUpdateTime.UniversalTime = 0;

	m_id = 0;
	m_isActive = false;
}
