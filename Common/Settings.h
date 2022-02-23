#pragma once

#define PROCESS_MESH
//#undef PROCESS_MESH

#define SCALE_POSITIONS
#undef SCALE_POSITIONS

#ifdef SCALE_POSITIONS
#define TRANSFORM_POSITIONS
#undef TRANSFORM_POSITIONS

#define WRITE_VERTICES
#undef WRITE_VERTICES
#endif

#define HIGHLIGHT_NEW_MESHES
#undef HIGHLIGHT_NEW_MESHES

// If you are using a very high detail setting with spatial mapping, it can be beneficial
// to use a 32-bit unsigned integer format for indices instead of the default 16-bit. 
#define USE_32BIT_INDICES
#undef USE_32BIT_INDICES

// #TODO
// bounding volumes
// move write vertices into helper file
// move mesh processing into separate file
// export function with hand input (choose if export in one or multiple .obj files?)
// Use scene understanding to extract measurements of objects? (in order to get some practical functionality of the device)
// Reimplement update interval time
// Implement coloring-options of surfaces

namespace Options {
	bool const DRAW_WIREFRAME_DEFAULT = false;
	double const MAX_TRIANGLES_PER_CUBIC_METER = 2000; // Low -> 100, Default -> 500, Med -> 750, High -> 2000 (1200)
	float const MAX_INACTIVE_MESH_TIME = 60.0f * 5.0f;
	float const MESH_FADE_IN_TIME = 2.0f;
	bool const INCLUDE_VERTEX_NORMALS = true;
}

// Note that it is possible to set multiple bounding volumes. Pseudocode:
//     m_surfaceObserver->SetBoundingVolumes(/* iterable collection of bounding volumes*/);
//
// It is also possible to use other bounding shapes - such as a view frustum. Pseudocode:
//     SpatialBoundingVolume^ bounds = SpatialBoundingVolume::FromFrustum(coordinateSystem, viewFrustum);
//     m_surfaceObserver->SetBoundingVolume(bounds);