#include "NavData.h"

#include <IO/BinaryReader.h>
#include <DetourNavMeshQuery.h>

namespace AI
{

bool CNavData::LoadFromStream(IO::IStream& Stream)
{
	Clear();

	IO::CBinaryReader Reader(Stream);
	int NMDataSize = Reader.Read<int>();
	U8* pData = (U8*)dtAlloc(NMDataSize, DT_ALLOC_PERM);
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

	int RegionCount = Reader.Read<int>();
	Regions.BeginAdd(RegionCount);
	for (int RIdx = 0; RIdx < RegionCount; ++RIdx)
	{
		CNavRegion& Region = Regions.Add(Reader.Read<CStrID>());
		Region.SetSize(Reader.Read<int>());
		Stream.Read(Region.GetPtr(), sizeof(dtPolyRef) * Region.GetCount());
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
			pNavMeshQuery[i] = nullptr;
		}

	if (pNavMesh)
	{
		dtFreeNavMesh(pNavMesh);
		pNavMesh = nullptr;
	}
}
//---------------------------------------------------------------------

}