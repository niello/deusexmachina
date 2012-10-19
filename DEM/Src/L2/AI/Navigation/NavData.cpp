#include "NavData.h"

#include <Data/Stream.h>
#include <Data/BinaryReader.h>
#include <DetourNavMeshQuery.h>

namespace AI
{

bool CNavData::LoadFromStream(Data::CStream& Stream)
{
	Clear();

	int NMDataSize = Stream.Get<int>();
	uchar* pData = (uchar*)dtAlloc(NMDataSize, DT_ALLOC_PERM);
	int BytesRead = Stream.Read(pData, NMDataSize);
	n_assert(BytesRead == NMDataSize);

	pNavMesh = dtAllocNavMesh();
	if (!pNavMesh)
	{
		dtFree(pData);
		FAIL;
	}
	if (dtStatusFailed(pNavMesh->init(pData, NMDataSize, DT_TILE_FREE_DATA)))
	{
		dtFreeNavMesh(pNavMesh);
		dtFree(pData);
		FAIL;
	}

	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
	{
		pNavMeshQuery[i] = dtAllocNavMeshQuery();
		if (!pNavMeshQuery[i] || dtStatusFailed(pNavMeshQuery[i]->init(pNavMesh, MAX_COMMON_NODES)))
		{
			Clear();
			FAIL;
		}
	}

	Data::CBinaryReader Reader(Stream);
	int RegionCount = Stream.Get<int>();
	Regions.BeginAdd(RegionCount);
	for (int RIdx = 0; RIdx < RegionCount; ++RIdx)
	{
		CNavRegion& Region = Regions.Add(Reader.Read<CStrID>());
		Region.SetSize(Stream.Get<int>());
		Stream.Read(Region.GetPtr(), sizeof(dtPolyRef) * Region.Size());
	}
	Regions.EndAdd();
	OK;
}
//---------------------------------------------------------------------

void CNavData::Clear()
{
	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		if (pNavMeshQuery[i])
		{
			dtFreeNavMeshQuery(pNavMeshQuery[i]);
			pNavMeshQuery[i] = NULL;
		}

	if (pNavMesh)
	{
		dtFreeNavMesh(pNavMesh);
		pNavMesh = NULL;
	}
}
//---------------------------------------------------------------------

}