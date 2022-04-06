#pragma once

using namespace Windows::Perception::Spatial;
using namespace Windows::Foundation::Numerics;

// If you are using a very high detail setting with spatial mapping, it can be beneficial
// to use a 32-bit unsigned integer format for indices instead of the default 16-bit. 
#define USE_32BIT_INDICES
#undef USE_32BIT_INDICES

namespace Settings {
	bool const DRAW_WIREFRAME_INIT_VALUE = true;
	double const MAX_TRIANGLE_RES = 3500;
	float const MAX_INACTIVE_MESH_TIME = 60.0f * 10.0f;
	float const MESH_FADE_IN_TIME = 1.5f;
	bool const INCLUDE_VERTEX_NORMALS = true;


	// Note that it is possible to set multiple bounding volumes with SetBoundingVolumes(*Iterable collection*);
	// See "HoloLens 1 sensor evaluation.pdf" and "IEEEM - Technical Evaluation of HoloLens for Multimedia: A First Look.pdf" for optimal bounding limits
	SpatialBoundingBox const BOUNDING_BOX = {
			{0.f, 0.f, 0.f}, // At device-position
			{3.f, 3.f, 5.f}
	};

	bool const MOCK_IMPROVEMENT = false;
}