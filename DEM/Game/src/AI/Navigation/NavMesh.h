#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>
#include <DetourNavMesh.h>
#include <map>

// Navigation mesh for predefined agent parameters

namespace DEM::AI
{
using CNavRegion = std::unique_ptr<dtPolyRef[]>;

class CNavMesh : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	float                        _AgentRadius = 0.f;
	float                        _AgentHeight = 0.f;

	dtNavMesh*                   _pNavMesh = nullptr;

	std::map<CStrID, CNavRegion> _Regions;

public:

	virtual ~CNavMesh() override;

	virtual bool IsResourceValid() const override { return !!_pNavMesh; }

	float GetAgentRadius() const { return _AgentRadius; }
	float GetAgentHeight() const { return _AgentHeight; }
};

}
