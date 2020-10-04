#pragma once
#include <Data/RefCounted.h>
#include <AI/Navigation/NavMesh.h>
#include <Game/ECS/Entity.h>

// Navigation map used by agents to navigate over the game level

namespace Resources
{
	using PResource = Ptr<class CResource>;
}

namespace DEM::AI
{
using PNavMap = Ptr<class CNavMap>;

class CNavMap : public ::Data::CRefCounted
{
protected:

	// Stored to be able to sort navmaps in a level even when namesh resource is not loaded
	//???need? will ever load navmesh on the fly? for optional big enemy?
	float                _AgentRadius;
	float                _AgentHeight;

	Resources::PResource _NavMesh;

	std::vector<std::pair<dtPolyRef, Game::HEntity>> _Controllers; // Sorted by polyref

public:

	CNavMap(float AgentRadius, float AgentHeight, Resources::PResource NavMesh);
	virtual ~CNavMap() override;

	float             GetAgentRadius() const { return _AgentRadius; }
	float             GetAgentHeight() const { return _AgentHeight; }
	const CNavRegion* FindRegion(CStrID ID) const;
	void              SetRegionController(CStrID RegionID, Game::HEntity Controller);
	void              RemoveController(Game::HEntity Controller);
	Game::HEntity     GetPolyController(dtPolyRef PolyRef) const;
	CNavMesh*         GetNavMesh() const;
	dtNavMesh*        GetDetourNavMesh() const;
};

}
