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

#include <string>
#include <fstream>

#include <DirectXPackedVector.h>

#include "Common\DirectXHelper.h"
#include "RealtimeSurfaceMeshRenderer.h"
#include "GetDataFromIBuffer.h"

using namespace SpatialMapping;

using namespace Concurrency;
using namespace DX;
using namespace DirectX;
using namespace PackedVector;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Perception::Spatial;
using namespace Windows::Perception::Spatial::Surfaces;
using namespace Windows::Storage;
using namespace Platform;

RealtimeSurfaceMeshRenderer::RealtimeSurfaceMeshRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
};

// Called once per frame, maintains and updates the mesh collection.
void RealtimeSurfaceMeshRenderer::Update(
	DX::StepTimer const& timer,
	SpatialCoordinateSystem^ coordinateSystem
)
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);

	const float timeElapsed = static_cast<float>(timer.GetTotalSeconds());

	// Update meshes as needed, based on the current coordinate system.
	// Also remove meshes that are inactive for too long.
	for (auto iter = m_meshCollection.begin(); iter != m_meshCollection.end();)
	{
		auto& pair = *iter;
		auto& surfaceMesh = pair.second;

		// Update the surface mesh.
		surfaceMesh.UpdateTransform(
			m_deviceResources->GetD3DDevice(),
			m_deviceResources->GetD3DDeviceContext(),
			timer,
			coordinateSystem
		);

		// Check to see if the mesh has expired.
		float const lastActiveTime = surfaceMesh.GetLastActiveTime();
		float const inactiveDuration = timeElapsed - lastActiveTime;
		if (inactiveDuration > c_maxInactiveMeshTime)
		{
			// Surface mesh is expired.
			m_meshCollection.erase(iter++);
		}
		else
		{
			++iter;
		}
	};
}

void RealtimeSurfaceMeshRenderer::AddSurface(Guid id, SpatialSurfaceInfo^ newSurface)
{
#ifdef HIGHLIGHT_NEW_MESHES
	auto fadeInMeshTask = AddOrUpdateSurfaceAsync(id, newSurface).then([this, id]()
		{
			if (HasSurface(id))
			{
				std::lock_guard<std::mutex> guard(m_meshCollectionLock);

				// In this example, new surfaces are treated differently by highlighting them in a different
				// color. This allows you to observe changes in the spatial map that are due to new meshes,
				// as opposed to mesh updates.
				auto& surfaceMesh = m_meshCollection[id];
				surfaceMesh.SetColorFadeTimer(c_surfaceMeshFadeInTime);
			}
		});
#else
	AddOrUpdateSurfaceAsync(id, newSurface);
#endif
}

void RealtimeSurfaceMeshRenderer::UpdateSurface(Guid id, SpatialSurfaceInfo^ newSurface)
{
	AddOrUpdateSurfaceAsync(id, newSurface);
}

Concurrency::task<void> RealtimeSurfaceMeshRenderer::AddOrUpdateSurfaceAsync(Guid id, SpatialSurfaceInfo^ newSurface)
{
	auto options = ref new SpatialSurfaceMeshOptions();
	options->IncludeVertexNormals = Options::INCLUDE_VERTEX_NORMALS;

	auto computeMeshTask = create_task(newSurface->TryComputeLatestMeshAsync(m_maxTrianglesPerCubicMeter, options));
	auto processMeshTask = computeMeshTask.then([this, id](SpatialSurfaceMesh^ mesh)
		{
			if (mesh != nullptr)
			{
				std::lock_guard<std::mutex> guard(m_meshCollectionLock);

				auto& surfaceMesh = m_meshCollection[id];
				surfaceMesh.UpdateSurface(mesh);
				surfaceMesh.SetIsActive(true);
			}
		}, task_continuation_context::use_current());

	return processMeshTask;
}

void RealtimeSurfaceMeshRenderer::RemoveSurface(Guid id)
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);
	m_meshCollection.erase(id);
}

void RealtimeSurfaceMeshRenderer::ClearSurfaces()
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);
	m_meshCollection.clear();
}

void RealtimeSurfaceMeshRenderer::HideInactiveMeshes(IMapView<Guid, SpatialSurfaceInfo^>^ const& surfaceCollection)
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);

	// Hide surfaces that aren't actively listed in the surface collection.
	for (auto& pair : m_meshCollection)
	{
		const auto& id = pair.first;
		auto& surfaceMesh = pair.second;

		surfaceMesh.SetIsActive(surfaceCollection->HasKey(id));
	};
}

// Renders one frame using the vertex, geometry, and pixel shaders.
void RealtimeSurfaceMeshRenderer::Render(bool isStereo, bool useWireframe)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
	);

	// The constant buffer is per-mesh, and will be set for each one individually.
	if (!m_usingVprtShaders)
	{
		// Attach the passthrough geometry shader.
		context->GSSetShader(
			m_geometryShader.Get(),
			nullptr,
			0
		);
	}

	if (useWireframe)
	{
		// Use a wireframe rasterizer state.
		m_deviceResources->GetD3DDeviceContext()->RSSetState(m_wireframeRasterizerState.Get());

		// Attach a pixel shader to render a solid color wireframe.
		context->PSSetShader(
			m_colorPixelShader.Get(),
			nullptr,
			0
		);
	}
	else
	{
		// Use the default rasterizer state.
		m_deviceResources->GetD3DDeviceContext()->RSSetState(m_defaultRasterizerState.Get());

		// Attach a pixel shader that can do lighting.
		context->PSSetShader(
			m_lightingPixelShader.Get(),
			nullptr,
			0
		);
	}

	{
		std::lock_guard<std::mutex> guard(m_meshCollectionLock);

		// Draw the meshes.
		auto device = m_deviceResources->GetD3DDevice();
		for (auto& pair : m_meshCollection)
		{
			auto& id = pair.first;
			auto& surfaceMesh = pair.second;

			surfaceMesh.Draw(device, context, m_usingVprtShaders, isStereo);
		}
	}
}

void RealtimeSurfaceMeshRenderer::CreateDeviceDependentResources()
{
	m_meshCollection.clear();
	m_usingVprtShaders = m_deviceResources->GetDeviceSupportsVprt();

	// On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
	// VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
	// we can avoid using a pass-through geometry shader to set the render
	// target array index, thus avoiding any overhead that would be
	// incurred by setting the geometry shader stage.
	std::wstring vertexShaderFileName = m_usingVprtShaders ? L"ms-appx:///SurfaceVprtVertexShader.cso" : L"ms-appx:///SurfaceVertexShader.cso";

	// Load shaders asynchronously.
	task<std::vector<byte>> loadVSTask = DX::ReadDataAsync(vertexShaderFileName);
	task<std::vector<byte>> loadLightingPSTask = DX::ReadDataAsync(L"ms-appx:///SimpleLightingPixelShader.cso");
	task<std::vector<byte>> loadWireframePSTask = DX::ReadDataAsync(L"ms-appx:///SolidColorPixelShader.cso");

	task<std::vector<byte>> loadGSTask;
	if (!m_usingVprtShaders)
	{
		// Load the pass-through geometry shader.
		loadGSTask = DX::ReadDataAsync(L"ms-appx:///SurfaceGeometryShader.cso");
	}

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R8G8B8A8_SNORM,     1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
		});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createLightingPSTask = loadLightingPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_lightingPixelShader
			)
		);
		});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createWireframePSTask = loadWireframePSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_colorPixelShader
			)
		);
		});

	task<void> createGSTask;
	if (!m_usingVprtShaders)
	{
		// After the pass-through geometry shader file is loaded, create the shader.
		createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData)
			{
				DX::ThrowIfFailed(
					m_deviceResources->GetD3DDevice()->CreateGeometryShader(
						&fileData[0],
						fileData.size(),
						nullptr,
						&m_geometryShader
					)
				);
			});
	}

	// Once all shaders are loaded, create the mesh.
	task<void> shaderTaskGroup = m_usingVprtShaders ?
		(createLightingPSTask && createWireframePSTask && createVSTask) :
		(createLightingPSTask && createWireframePSTask && createVSTask && createGSTask);

	// Once the cube is loaded, the object is ready to be rendered.
	auto finishLoadingTask = shaderTaskGroup.then([this]() {

		// Recreate device-based surface mesh resources.
		std::lock_guard<std::mutex> guard(m_meshCollectionLock);
		for (auto& iter : m_meshCollection)
		{
			iter.second.ReleaseDeviceDependentResources();
			iter.second.CreateDeviceDependentResources(m_deviceResources->GetD3DDevice());
		}

		// Create a default rasterizer state descriptor.
		D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);

		// Create the default rasterizer state.
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_defaultRasterizerState.GetAddressOf());

		// Change settings for wireframe rasterization.
		rasterizerDesc.AntialiasedLineEnable = true;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;

		// Create a wireframe rasterizer state.
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_wireframeRasterizerState.GetAddressOf());

		m_loadingComplete = true;
		});
}

void RealtimeSurfaceMeshRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;

	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_geometryShader.Reset();
	m_lightingPixelShader.Reset();
	m_colorPixelShader.Reset();

	m_defaultRasterizerState.Reset();
	m_wireframeRasterizerState.Reset();

	std::lock_guard<std::mutex> guard(m_meshCollectionLock);
	for (auto& iter : m_meshCollection)
	{
		iter.second.ReleaseDeviceDependentResources();
	}
}

bool RealtimeSurfaceMeshRenderer::HasSurface(Platform::Guid id)
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);
	return m_meshCollection.find(id) != m_meshCollection.end();
}

Windows::Foundation::DateTime RealtimeSurfaceMeshRenderer::GetLastUpdateTime(Platform::Guid id)
{
	std::lock_guard<std::mutex> guard(m_meshCollectionLock);
	auto& meshIter = m_meshCollection.find(id);
	if (meshIter != m_meshCollection.end())
	{
		auto const& mesh = meshIter->second;
		return mesh.GetLastUpdateTime();
	}
	else
	{
		static const Windows::Foundation::DateTime zero;
		return zero;
	}
}

void RealtimeSurfaceMeshRenderer::ExportMeshes(SpatialCoordinateSystem^& const worldCoordinateSystem)
{
	//m_exportMeshesTask.then([this, worldCoordinateSystem]()
		//{
	ApplicationData::Current->LocalFolder->CreateFolderAsync("Meshes", CreationCollisionOption::FailIfExists);
	String^ folder = ApplicationData::Current->LocalFolder->Path + "\\Meshes";
	std::wstring folderW(folder->Begin());
	std::string folderA(folderW.begin(), folderW.end());
	const char* charStr = folderA.c_str();
	char file[512];
	std::snprintf(file, 512, "%s\\meshes.obj", charStr);
	std::ofstream fileOut(file, std::ios::out);

	std::lock_guard<std::mutex> guard(m_meshCollectionLock);

	for (auto& const kvPair : m_meshCollection)
	{
		auto& const id = kvPair.first;
		auto& const mesh = kvPair.second;

		const SurfaceMeshProperties* meshProperties = mesh.GetSurfaceMeshProperties();
		SpatialCoordinateSystem^ const meshCoordSystem = meshProperties->coordinateSystem;
		IBox<float4x4>^ const surfCoordSysToWorld = meshCoordSystem->TryGetTransformTo(worldCoordinateSystem);

		if (surfCoordSysToWorld != nullptr) {
			auto const positionsIBuffer = mesh.GetPositionsIBuffer();
			auto const normalsIBuffer = mesh.GetNormalsIBuffer();
			auto const indexIBuffer = mesh.GetIndexIBuffer();

			if (positionsIBuffer != nullptr && normalsIBuffer != nullptr && indexIBuffer != nullptr) {
				fileOut << "\no mesh_" << meshProperties->id << "\n\n";

				XMSHORTN4* const positions = GetDataFromIBuffer<XMSHORTN4>(*positionsIBuffer);
				float3 const posScale = meshProperties->vertexPositionScale;
				for (int i = 0; i < meshProperties->posCount; i++)
				{
					XMFLOAT4 p;
					XMVECTOR const vec = XMLoadShortN4(&positions[i]);
					XMStoreFloat4(&p, vec);

					XMFLOAT4 const ps = XMFLOAT4(p.x * posScale.x, p.y * posScale.y, p.z * posScale.z, p.w);
					float3 const t = transform(float3(ps.x, ps.y, ps.z), surfCoordSysToWorld->Value);
					XMFLOAT4 const pst = XMFLOAT4(t.x, t.y, t.z, ps.w);
					fileOut << "v " << pst.x << " " << pst.y << " " << pst.z << "\n";
				}

				fileOut << "\n";

				XMBYTEN4* const normals = GetDataFromIBuffer<XMBYTEN4>(*normalsIBuffer);
				for (int i = 0; i < meshProperties->normalCount; i++)
				{
					XMFLOAT4 n;
					XMVECTOR const vec = XMLoadByteN4(&normals[i]);
					XMStoreFloat4(&n, vec);

					fileOut << "vn " << n.x << " " << n.y << " " << n.z << "\n";
				}

				fileOut << "\n";

#ifdef USE_32BIT_INDICES
				uint32_t* const indices = GetDataFromIBuffer<uint32_t>(*indexIBuffer);
				for (int i = 0; i < meshProperties->indexCount; i += 3)
				{
					// +1 to get .obj format
					uint32_t const iOne = indices[i] + 1;
					uint32_t const iTwo = indices[i + 1] + 1;
					uint32_t const iThree = indices[i + 2] + 1;

					fileOut << "f " << iOne << " " << iTwo << " " << iThree << "\n";
				}
#else
				uint16_t* const indices = GetDataFromIBuffer<uint16_t>(*indexIBuffer);
				for (int i = 0; i < meshProperties->indexCount; i += 3)
				{
					// +1 to get .obj format
					uint16_t const iOne = indices[i] + 1;
					uint16_t const iTwo = indices[i + 1] + 1;
					uint16_t const iThree = indices[i + 2] + 1;

					fileOut << "f " << iOne << " " << iTwo << " " << iThree << "\n";
				}
#endif // USE_32BIT_INDICES
			}
		}
		fileOut.close();
	}
	SetIsExportingMeshes(false);
	//});
}