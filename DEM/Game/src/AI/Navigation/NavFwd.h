#pragma once
#include <Data/FixedArray.h>

// Navigation forward declarations

#ifdef DT_POLYREF64
#error "64-bit navigation poly refs aren't supported for now"
#else
typedef unsigned int dtPolyRef;
#endif

class dtNavMesh;

namespace DEM::AI
{
	using CNavRegion = CFixedArray<dtPolyRef>;
}
