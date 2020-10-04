#include "NavMap.h"
#include <Resources/Resource.h>

namespace DEM::AI
{

CNavMap::CNavMap(float AgentRadius, float AgentHeight, Resources::PResource NavMesh)
	: _AgentRadius(AgentRadius)
	, _AgentHeight(AgentHeight)
	, _NavMesh(std::move(NavMesh))
{
}
//---------------------------------------------------------------------

CNavMap::~CNavMap() = default;
//---------------------------------------------------------------------

const CNavRegion* CNavMap::FindRegion(CStrID ID) const
{
	auto pNavMesh = GetNavMesh();
	return pNavMesh ? pNavMesh->FindRegion(ID) : nullptr;
}
//---------------------------------------------------------------------

void CNavMap::SetRegionController(CStrID RegionID, Game::HEntity Controller)
{
	auto pRegion = FindRegion(RegionID);
	if (!pRegion) return;

	for (auto PolyRef : *pRegion)
	{
		auto It = std::lower_bound(_Controllers.begin(), _Controllers.end(), PolyRef,
			[](const auto& Elm, dtPolyRef Value) { return Elm.first < Value; });
		if (It != _Controllers.end() && It->first == PolyRef)
		{
			if (Controller) It->second = Controller;
			else _Controllers.erase(It);
		}
		else if (Controller) _Controllers.insert(It, { PolyRef, Controller });
	}
}
//---------------------------------------------------------------------

void CNavMap::RemoveController(Game::HEntity Controller)
{
	_Controllers.erase(
		std::remove_if(_Controllers.begin(), _Controllers.end(), [Controller](const auto& Value) { return Value.second == Controller; }),
		_Controllers.end());
}
//---------------------------------------------------------------------

Game::HEntity CNavMap::GetPolyController(dtPolyRef PolyRef) const
{
	auto It = std::lower_bound(_Controllers.begin(), _Controllers.end(), PolyRef,
		[](const auto& Elm, dtPolyRef Value) { return Elm.first < Value; });
	return (It != _Controllers.end() && It->first == PolyRef) ? It->second : Game::HEntity{};
}
//---------------------------------------------------------------------

CNavMesh* CNavMap::GetNavMesh() const
{
	return _NavMesh ? _NavMesh->ValidateObject<CNavMesh>() : nullptr;
}
//---------------------------------------------------------------------

dtNavMesh* CNavMap::GetDetourNavMesh() const
{
	auto pNavMesh = GetNavMesh();
	return pNavMesh ? pNavMesh->GetDetourNavMesh() : nullptr;
}
//---------------------------------------------------------------------

}
