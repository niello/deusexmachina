#pragma once
#include <AI/Navigation/NavFwd.h>
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>
#include <map>

// Navigation mesh for predefined agent parameters

namespace DEM::AI
{

class CNavMesh : public Resources::CResourceObject
{
	RTTI_CLASS_DECL(DEM::AI::CNavMesh, Resources::CResourceObject);

protected:

	float                        _AgentRadius = 0.f;
	float                        _AgentHeight = 0.f;

	dtNavMesh*                   _pNavMesh = nullptr;
	std::vector<U8>              _NavMeshData;

	std::map<CStrID, CNavRegion> _Regions;

public:

	CNavMesh(float AgentRadius, float AgentHeight, std::vector<U8>&& RawData, std::map<CStrID, CNavRegion>&& Regions);
	virtual ~CNavMesh() override;

	virtual bool      IsResourceValid() const override { return !!_pNavMesh; }

	float             GetAgentRadius() const { return _AgentRadius; }
	float             GetAgentHeight() const { return _AgentHeight; }
	const CNavRegion* FindRegion(CStrID ID) const;

	dtNavMesh*        GetDetourNavMesh() const { return _pNavMesh; }
};

}
