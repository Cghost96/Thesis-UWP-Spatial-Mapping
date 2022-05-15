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

#pragma once

#include "Common\DeviceResources.h"
#include "Common\Settings.h"
#include "ShaderStructures.h"

#include <vector>

#include <ppltasks.h>
#include <sstream>
#include <memory>

namespace SpatialMapping
{
	struct SurfaceMeshProperties
	{
		Windows::Perception::Spatial::SpatialCoordinateSystem^ localCoordSystem = nullptr;
		Windows::Foundation::Numerics::float3 vertexPositionScale = Windows::Foundation::Numerics::float3::one();
		unsigned int vertexStride = 0;
		unsigned int normalStride = 0;
		unsigned int indexCount = 0;
		DXGI_FORMAT  indexFormat = DXGI_FORMAT_UNKNOWN;
	};

	class SurfaceMesh final
	{
	public:
		SurfaceMesh();
		~SurfaceMesh();

		void UpdateSurface(Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ surface);
		void UpdateTransform(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			DX::StepTimer const& timer,
			Windows::Perception::Spatial::SpatialCoordinateSystem^ baseCoordinateSystem
		);
		void Draw(ID3D11Device* device, ID3D11DeviceContext* context, bool usingVprtShaders, bool isStereo);
		void UpdateVertexResources(ID3D11Device* device, Windows::Perception::Spatial::SpatialCoordinateSystem^ worldCoordSystem);
		void CreateDeviceDependentResources(ID3D11Device* device);
		void ReleaseVertexResources();
		void ReleaseDeviceDependentResources();

#ifdef USE_32BIT_INDICES
		using IndexFormat = uint32_t;
#else
		using IndexFormat = uint16_t;
#endif
		const bool& IsActive()       const { return m_isActive; }
		const float& LastActiveTime() const { return m_lastActiveTime; }
		const Windows::Foundation::DateTime& LastUpdateTime() const { return m_lastUpdateTime; }
		const std::vector<Windows::Foundation::Numerics::float3>* PositionsTransformed() const { return &m_positionsTransformed; }
		const std::vector<Windows::Foundation::Numerics::float3>* PositionsNotTransformed() const { return &m_positionsNotTransformed; }
		const std::vector<Windows::Foundation::Numerics::float3>* FaceNormals() const { return &m_faceNormals; }
		const std::vector<IndexFormat>* Indices() const { return &m_indices; }
		const SurfaceMeshProperties* GetSurfaceMeshProperties() const { return &m_meshProperties; }
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexPositions() const { return m_vertexPositionsBuffer; }
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexNormals() const { return m_vertexNormalsBuffer; }
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetTriangleIndices() const { return m_triangleIndicesBuffer; }


		void IsActive(const bool& isActive) { m_isActive = isActive; }
		void ColorFadeTimer(const float& duration) {
			m_colorFadeTimeout = duration;
			m_colorFadeTimer = 0.f;
		}
		void ShuttingDown(const bool val) { m_isShuttingDown = val; }
		bool Expired() const { return m_isExpired; }
		void Expired(const bool val) {
			m_isExpired = val;
			m_isActive = !val;
		}

	private:
		void SwapVertexBuffers();
		void CreateDirectXBuffer(
			ID3D11Device* device,
			D3D11_BIND_FLAG binding,
			Windows::Storage::Streams::IBuffer^ buffer,
			ID3D11Buffer** target
		);

		concurrency::task<void> m_updateVertexResourcesTask = concurrency::task_from_result();

		Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ m_pendingSurfaceMesh = nullptr;
		Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ m_surfaceMesh = nullptr;

		std::vector<float3> m_positionsTransformed;
		std::vector<float3> m_positionsNotTransformed;
		std::vector<float3> m_faceNormals;
		std::vector<IndexFormat> m_indices;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexPositionsBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexNormalsBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_triangleIndicesBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedVertexPositionsBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedVertexNormalsBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedTriangleIndicesBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelTransformBuffer;

		Windows::Foundation::DateTime m_lastUpdateTime;

		SurfaceMeshProperties m_meshProperties;
		SurfaceMeshProperties m_updatedMeshProperties;

		ModelNormalConstantBuffer m_constantBufferData;

		bool   m_constantBufferCreated = false;
		bool   m_loadingComplete = false;
		bool   m_updateReady = false;
		bool   m_isActive = false;
		bool   m_isShuttingDown = false;
		bool   m_isExpired = false;
		float  m_lastActiveTime = -1.f;
		float  m_colorFadeTimer = -1.f;
		float  m_colorFadeTimeout = -1.f;

		std::mutex m_meshResourcesMutex;
	};
}