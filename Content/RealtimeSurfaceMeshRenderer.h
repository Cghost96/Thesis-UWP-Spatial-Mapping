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
#include "Common\StepTimer.h"
#include "Content\SurfaceMesh.h"
#include "Content\ShaderStructures.h"

#include <memory>
#include <unordered_map>
#include <ppltasks.h>

namespace SpatialMapping
{
	class RealtimeSurfaceMeshRenderer
	{
	public:
		RealtimeSurfaceMeshRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(
			DX::StepTimer const& timer,
			Windows::Perception::Spatial::SpatialCoordinateSystem^ coordinateSystem
		);
		void Render(bool isStereo, bool useWireframe);

		bool HasSurface(int const id);
		void AddSurface(int const id, Windows::Perception::Spatial::Surfaces::SpatialSurfaceInfo^ newSurface);
		void UpdateSurface(int const id, Windows::Perception::Spatial::Surfaces::SpatialSurfaceInfo^ newSurface);

		Windows::Foundation::DateTime LastUpdateTime(int const id);

		void HideInactiveMeshes(
			std::unordered_map<int, Platform::Guid> const& observedIDs,
			Windows::Foundation::Collections::IMapView<Platform::Guid,
			Windows::Perception::Spatial::Surfaces::SpatialSurfaceInfo^>^ const& surfaceCollection);

		std::unordered_map<int, SpatialMapping::SurfaceMesh>* MeshCollection() { return &m_meshCollection; }

	private:
		Concurrency::task<void> AddOrUpdateSurfaceAsync(int const id, Windows::Perception::Spatial::Surfaces::SpatialSurfaceInfo^ newSurface);

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources>            m_deviceResources;

		// Direct3D resources for SR mesh rendering pipeline.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>    m_geometryShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_lightingPixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_colorPixelShader;

		std::unordered_map<int, SurfaceMesh> m_meshCollection;

		// A way to lock map access.
		std::mutex                                      m_meshCollectionLock;

		// Level of detail setting. The number of triangles that the system is allowed to provide per cubic meter.
		double const                                    m_maxTrianglesPerCubicMeter = Settings::MAX_TRIANGLES_PER_CUBIC_METER;

		// If the current D3D Device supports VPRT, we can avoid using a geometry
		// shader just to set the render target array index.
		bool                                            m_usingVprtShaders = false;

		// Rasterizer states, for different rendering modes.
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_defaultRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_wireframeRasterizerState;

		// The duration of time, in seconds, a mesh is allowed to remain inactive before deletion.
		const float m_maxInactiveMeshTime = Settings::MAX_INACTIVE_MESH_TIME;

		// The duration of time, in seconds, taken for a new surface mesh to fade in on-screen.
		const float m_surfaceMeshFadeInTime = Settings::MESH_FADE_IN_TIME;

		bool m_loadingComplete;
	};
};

