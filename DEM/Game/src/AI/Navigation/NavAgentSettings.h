#pragma once
#include <Resources/ResourceObject.h>
#include <DetourNavMeshQuery.h>
#include <map>

// Navigation mesh traversal costs and rules for a certain type of agents

namespace DEM::AI
{
using PTraversalController = Ptr<class CTraversalController>;

class CNavAgentSettings : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	dtQueryFilter                              _Filter;
	std::vector<DEM::AI::PTraversalController> _Controllers;

public:

	CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalController>&& Controllers);
	virtual ~CNavAgentSettings() override;

	virtual bool          IsResourceValid() const override { return true; }

	CTraversalController* FindController(unsigned char AreaType, dtPolyRef PolyRef) const;
	const dtQueryFilter*  GetQueryFilter() const { return &_Filter; }
};

typedef Ptr<CNavAgentSettings> PNavAgentSettings;

}
