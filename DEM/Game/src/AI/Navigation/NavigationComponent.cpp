#include "NavMesh.h"
#include <DetourNavMesh.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(DEM::AI::CNavMesh, Resources::CResourceObject);

CNavMesh::CNavMesh(float AgentRadius, float AgentHeight, std::vector<U8>&& RawData, std::map<CStrID, CNavRegion>&& Regions)
	: _AgentRadius(AgentRadius)
	, _AgentHeight(AgentHeight)
	, _NavMeshData(std::move(RawData))
	, _Regions(std::move(Regions))
{
	if (_pNavMesh = dtAllocNavMesh())
		if (dtStatusFailed(_pNavMesh->init(_NavMeshData.data(), _NavMeshData.size(), 0)))
			dtFreeNavMesh(_pNavMesh);
}
//---------------------------------------------------------------------

CNavMesh::~CNavMesh()
{
	if (_pNavMesh) dtFreeNavMesh(_pNavMesh);
}
//---------------------------------------------------------------------

const CNavRegion* CNavMesh::FindRegion(CStrID ID) const
{
	auto It = _Regions.find(ID);
	return (It == _Regions.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

}
