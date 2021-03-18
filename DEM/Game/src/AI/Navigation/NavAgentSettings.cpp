#include "NavAgentSettings.h"
#include <AI/Navigation/TraversalAction.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/NavControllerComponent.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::AI
{

CNavAgentSettings::CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalAction>&& Actions, std::vector<bool>&& UseControllers)
	: _Actions(std::move(Actions))
	, _UseControllers(std::move(UseControllers))
{
	if (_Actions.size() > 64) _Actions.resize(64);
	if (_UseControllers.size() > 64) _UseControllers.resize(64);
	_Actions.shrink_to_fit();
	_UseControllers.shrink_to_fit();

	// NB: it is not recommended in Detour docs to use costs less than 1.0
	for (auto [Area, Cost] : Costs)
		if (Cost > 1.f)
			_Filter.setAreaCost(Area, Cost);

	// FIXME: share! To navigation config or engine level constant?
	constexpr U16 NAV_FLAG_LOCKED = 2;
	_Filter.setExcludeFlags(NAV_FLAG_LOCKED);
}
//---------------------------------------------------------------------

CNavAgentSettings::~CNavAgentSettings() = default;
//---------------------------------------------------------------------

CTraversalAction* CNavAgentSettings::FindAction(const Game::CGameWorld& World, const CNavAgentComponent& Agent, U8 AreaType, dtPolyRef PolyRef, Game::HEntity* pOutController) const
{
	if (Agent.NavMap && IsAreaControllable(AreaType))
	{
		Game::HEntity ControllerID = Agent.NavMap->GetPolyController(PolyRef);
		if (auto pController = World.FindComponent<const CNavControllerComponent>(ControllerID))
		{
			if (pOutController) *pOutController = ControllerID;
			return pController->Action.Get();
		}
	}

	if (pOutController) *pOutController = {};
	return (AreaType < _Actions.size()) ? _Actions[AreaType].Get() : nullptr;
}
//---------------------------------------------------------------------

}
