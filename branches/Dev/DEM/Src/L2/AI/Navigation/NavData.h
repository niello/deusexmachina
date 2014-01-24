#pragma once
#ifndef __DEM_L2_AI_NAV_DATA_H__
#define __DEM_L2_AI_NAV_DATA_H__

#include <Data/StringID.h>
#include <Data/FixedArray.h>
#include <Data/Dictionary.h>
#include <DetourNavMesh.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysWorld),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

static const int MAX_NAV_PATH = 256;
static const int MAX_ITERS_PER_UPDATE = 100;
static const int MAX_PATHQUEUE_NODES = 4096;
static const int MAX_COMMON_NODES = 512;

class dtNavMeshQuery;

namespace IO
{
	class CStream;
}

namespace AI
{
typedef CFixedArray<dtPolyRef> CNavRegion;

#define NAV_FLAG_NORMAL	0x01	// Normal poly (poly must have at least 1 flag set to be passable)
#define NAV_FLAG_LOCKED	0x02	// Poly is locked by door, obstacle or any other physical/collision object 

class CNavData
{
public:

	float						AgentRadius;
	float						AgentHeight;

	dtNavMesh*					pNavMesh;
	dtNavMeshQuery*				pNavMeshQuery[DEM_THREAD_COUNT]; // [0] is sync, main query

	CDict<CStrID, CNavRegion>	Regions;

	CNavData(): pNavMesh(NULL) { memset(pNavMeshQuery, 0, sizeof(pNavMeshQuery)); }

	bool LoadFromStream(IO::CStream& Stream);
	void Clear();
};

}

#endif