#include "NavMeshLoaderNM.h"
#include <AI/Navigation/NavMesh.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CNavMeshLoaderNM::GetResultType() const
{
	return DEM::AI::CNavMesh::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CNavMeshLoaderNM::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'NAVM') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	float R, H;
	if (!Reader.Read(R)) return nullptr;
	if (!Reader.Read(H)) return nullptr;
	//???need other agent params too?

	U32 DataSize;
	if (!Reader.Read(DataSize) || !DataSize) return nullptr;

	std::vector<U8> RawData(DataSize);
	if (Stream->Read(RawData.data(), DataSize) != DataSize) return nullptr;

	U32 RegionCount;
	if (!Reader.Read<U32>(RegionCount)) return nullptr;

	std::map<CStrID, DEM::AI::CNavRegion> Regions;
	for (U32 i = 0; i < RegionCount; ++i)
	{
		CStrID ID;
		if (!Reader.Read(ID)) return nullptr;

		U32 PolyCount;
		if (!Reader.Read<U32>(PolyCount)) return nullptr;

		DEM::AI::CNavRegion Region(PolyCount);
		if (Stream->Read(Region.data(), PolyCount * sizeof(dtPolyRef)) != PolyCount * sizeof(dtPolyRef)) return nullptr;

		Regions.emplace(ID, std::move(Region));
	}

	return n_new(DEM::AI::CNavMesh(R, H, std::move(RawData), std::move(Regions)));
}
//---------------------------------------------------------------------

}
