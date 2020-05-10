#pragma once
#include <Resources/ResourceObject.h>
#include <DetourNavMeshQuery.h>

// Navigation mesh traversal costs and rules for a certain type of agents

namespace DEM::AI
{
class CTraversalController;

class CNavAgentSettings : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	dtQueryFilter Filter;

public:

	CNavAgentSettings();
	virtual ~CNavAgentSettings() override;

	virtual bool          IsResourceValid() const override { return true; }

	CTraversalController* FindController(unsigned char AreaType, dtPolyRef PolyRef) const;
	CTraversalController* GetRecoveryController() const;
	const dtQueryFilter*  GetQueryFilter() const { return &Filter; }
};

typedef Ptr<CNavAgentSettings> PNavAgentSettings;

}
