#include "NavAgentSettings.h"
#include <AI/Navigation/TraversalAction.h>
#include <AI/Navigation/NavControllerComponent.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(DEM::AI::CNavAgentSettings, Resources::CResourceObject);

CNavAgentSettings::CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalAction>&& Actions, std::vector<bool>&& UseSmartObjects)
	: _Actions(std::move(Actions))
	, _UseSmartObjects(std::move(UseSmartObjects))
{
	if (_Actions.size() > 64) _Actions.resize(64);
	if (_UseSmartObjects.size() > 64) _UseSmartObjects.resize(64);
	_Actions.shrink_to_fit();
	_UseSmartObjects.shrink_to_fit();

	// NB: it is not recommended in Detour docs to use costs less than 1.0
	for (auto [Area, Cost] : Costs)
		if (Cost > 1.f)
			_Filter.setAreaCost(Area, Cost);
}
//---------------------------------------------------------------------

CNavAgentSettings::~CNavAgentSettings() = default;
//---------------------------------------------------------------------

CTraversalAction* CNavAgentSettings::FindAction(const CNavAgentComponent& Agent, U8 AreaType, dtPolyRef PolyRef, Game::HEntity* pOutController) const
{
	const bool UseSmart = (AreaType < _UseSmartObjects.size()) ? _UseSmartObjects[AreaType] : false;
	if (UseSmart)
	{
		Game::HEntity Controller;

		NOT_IMPLEMENTED;
		// get navmesh from the agent (cache inside?)
		// find controller entity handle by PolyRef

		// find controller component, need world!
		// get action (need also agent pos?)
		// if action found, return it
	}

	if (pOutController) *pOutController = {};
	return (AreaType < _Actions.size()) ? _Actions[AreaType].Get() : nullptr;
}
//---------------------------------------------------------------------

}
