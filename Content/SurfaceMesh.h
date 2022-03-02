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

		inline static std::string meshFolderPath;
		inline static bool canExport = true;

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

		const bool& GetIsActive()       const { return m_isActive; }
		const float& GetLastActiveTime() const { return m_lastActiveTime; }
		const Windows::Foundation::DateTime& GetLastUpdateTime() const { return m_lastUpdateTime; }

		void SetIsActive(const bool& isActive) { m_isActive = isActive; }
		void SetColorFadeTimer(const float& duration) { m_colorFadeTimeout = duration; m_colorFadeTimer = 0.f; }

		const std::vector<Windows::Foundation::Numerics::float3>* GetExportPositions() const { return &m_exportPositions; }
		const SurfaceMeshProperties* GetSurfaceMeshProperties() const { return &m_meshProperties; }

		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexPositions() const { return m_vertexPositions; }
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexNormals() const { return m_vertexNormals; }
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetTriangleIndices() const { return m_triangleIndices; }
		bool FinishedExporting() const { return m_finishedExporting; }
		void FinishedExporting(bool val) { m_finishedExporting = val; }

	private:
		void SwapVertexBuffers();
		void CreateDirectXBuffer(
			ID3D11Device* device,
			D3D11_BIND_FLAG binding,
			Windows::Storage::Streams::IBuffer^ buffer,
			ID3D11Buffer** target
		);
		void CalculateRegression(
			Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ surface,
			Windows::Storage::Streams::IBuffer^& positions,
			Windows::Storage::Streams::IBuffer^& normals,
			Windows::Storage::Streams::IBuffer^& indices,
			Windows::Perception::Spatial::SpatialCoordinateSystem^ currentCoordSys);

		concurrency::task<void> m_updateVertexResourcesTask = concurrency::task_from_result();

		Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ m_pendingSurfaceMesh = nullptr;
		Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh^ m_surfaceMesh = nullptr;

		std::vector<float3> m_exportPositions;
#ifdef USE_32BIT_INDICES
		std::vector<uint32_t> m_exportIndices;
#else
		std::vector<uint16_t> m_exportIndices;
#endif

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexPositions;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexNormals;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_triangleIndices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedVertexPositions;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedVertexNormals;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_updatedTriangleIndices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelTransformBuffer;

		Windows::Foundation::DateTime m_lastUpdateTime;

		SurfaceMeshProperties m_meshProperties;
		SurfaceMeshProperties m_updatedMeshProperties;

		ModelNormalConstantBuffer m_constantBufferData;

		bool   m_constantBufferCreated = false;
		bool   m_loadingComplete = false;
		bool   m_updateReady = false;
		bool   m_isActive = false;
		float  m_lastActiveTime = -1.f;
		float  m_colorFadeTimer = -1.f;
		float  m_colorFadeTimeout = -1.f;

		std::mutex m_meshResourcesMutex;

		bool m_finishedExporting = true;
		std::mutex m_exportMutex;
	};
}
