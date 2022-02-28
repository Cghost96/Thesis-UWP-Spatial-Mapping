#pragma once

using namespace Windows::Perception::Spatial;
using namespace Windows::Foundation::Numerics;

#define PROCESS_MESH
//#undef PROCESS_MESH

#define SCALE_POSITIONS
#undef SCALE_POSITIONS

#ifdef SCALE_POSITIONS
#define TRANSFORM_POSITIONS
#undef TRANSFORM_POSITIONS

#define EXPORT_MESHES
#undef EXPORT_MESHES
#endif

#define HIGHLIGHT_NEW_MESHES
#undef HIGHLIGHT_NEW_MESHES

// If you are using a very high detail setting with spatial mapping, it can be beneficial
// to use a 32-bit unsigned integer format for indices instead of the default 16-bit. 
#define USE_32BIT_INDICES
#undef USE_32BIT_INDICES

#define USE_BOUNDING_BOX
//#undef USE_BOUNDING_BOX

#define USE_BOUNDING_FRUSTUM
#undef USE_BOUNDING_FRUSTUM

// #TODO Move write vertices into helper file
// #TODO Move mesh processing into separate file
// #TODO Remote computing
// #TODO Export function with hand input (export all in one file as separate objects)
// #TODO Use scene understanding to extract measurements of objects? (in order to get some practical functionality of the device)
// #TODO Implement coloring-options of surfaces

namespace Options {
	bool const DRAW_WIREFRAME_START_VALUE = true;
	double const MAX_TRIANGLES_PER_CUBIC_METER = 2000;
	float const MAX_INACTIVE_MESH_TIME = 60.0f * 5.0f;
	float const MESH_FADE_IN_TIME = 2.0f;
	bool const INCLUDE_VERTEX_NORMALS = true;


	// Note that it is possible to set multiple bounding volumes with SetBoundingVolumes(*Iterable collection*);
	// See "HoloLens 1 sensor evaluation.pdf" and "IEEEM - Technical Evaluation of HoloLens for Multimedia: A First Look.pdf"
	//   for optimal bounding limits
#ifdef USE_BOUNDING_BOX
	SpatialBoundingBox const BOUNDING_BOX = {
			{0.f, 0.f, 0.f}, // At device-position
			{5.f, 5.f, 5.f} // 2.5 meters on all sides
	};
#endif

#ifdef USE_BOUNDING_FRUSTUM
	SpatialBoundingFrustum const BOUNDING_FRUSTUM = {
		plane(), // Bottom
		plane(), // Far
		plane(), // Left
		plane(), // Near
		plane(), // Right
		plane()  // Top
	};
#endif
}