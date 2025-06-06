#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <DetourNavMeshQuery.h>
#include <map>

// Navigation mesh traversal costs and rules for a certain type of agents

namespace DEM::Game
{
	class CGameWorld;
}

namespace DEM::AI
{
using PTraversalAction = Ptr<class CTraversalAction>;
struct CNavAgentComponent;

class CNavAgentSettings : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::AI::CNavAgentSettings, DEM::Core::CObject);

protected:

	dtQueryFilter                          _Filter;
	std::vector<DEM::AI::PTraversalAction> _Actions;
	std::vector<bool>                      _UseControllers;

public:

	CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalAction>&& Actions, std::vector<bool>&& UseControllers);
	virtual ~CNavAgentSettings() override;

	CTraversalAction*    FindAction(const Game::CGameWorld& World, const CNavAgentComponent& Agent, U8 AreaType, dtPolyRef PolyRef, Game::HEntity* pOutController) const;
	const dtQueryFilter* GetQueryFilter() const { return &_Filter; }
	bool                 IsAreaControllable(U8 AreaType) const { return (AreaType < _UseControllers.size()) ? _UseControllers[AreaType] : false; }
};

typedef Ptr<CNavAgentSettings> PNavAgentSettings;

}
