#pragma once
#include <Resources/ResourceObject.h>
#include <Game/ECS/Entity.h>
#include <DetourNavMeshQuery.h>
#include <map>

// Navigation mesh traversal costs and rules for a certain type of agents

namespace DEM::AI
{
using PTraversalAction = Ptr<class CTraversalAction>;
struct CNavAgentComponent;

class CNavAgentSettings : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	dtQueryFilter                          _Filter;
	std::vector<DEM::AI::PTraversalAction> _Actions;
	std::vector<bool>                      _UseSmartObjects;

public:

	CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalAction>&& Actions, std::vector<bool>&& UseSmartObjects);
	virtual ~CNavAgentSettings() override;

	virtual bool         IsResourceValid() const override { return true; }

	CTraversalAction*    FindAction(const CNavAgentComponent& Agent, U8 AreaType, dtPolyRef PolyRef, Game::HEntity* pOutSmartObject) const;
	const dtQueryFilter* GetQueryFilter() const { return &_Filter; }
};

typedef Ptr<CNavAgentSettings> PNavAgentSettings;

}
