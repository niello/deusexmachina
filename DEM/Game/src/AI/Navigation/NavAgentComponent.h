#pragma once
#include <AI/Navigation/NavAgentSettings.h>
#include <Data/Ptr.h>
#include <Data/Metadata.h>
#include <Events/EventNative.h>
#include <Math/Vector3.h>
#include <DetourPathCorridor.h>

// Navigation component allows an entity to plan path along navigation mesh

namespace DEM::AI
{

class Navigate: public Events::CEventNative
{
	NATIVE_EVENT_DECL;

public:

	//!!!_Destination can instead be universal ITarget, with impls like "point", "entity", "nearest ally" etc!
	//???or navigation is always to point, and point is updated from target externally? strange.
	vector3 _Destination;
	float   _Speed = 0.f;
	// MinDistance, MaxDistance, MaxTargetOffset

	explicit Navigate(const vector3& Destination, float Speed) : _Destination(Destination), _Speed(Speed) {}
};

enum class ENavigationState : U8
{
	Idle = 0,  // No navigation is happening currently
	Requested, // Destination requested, but no path is planned
	Planning,  // Quick path built if possible, full path planning is in progress
	Following  // Full path is planned
};

enum class ENavigationMode : U8
{
	Recovery = 0, // Agent is outside the navmesh and will recovery to the surface unless Idle
	Surface,      // Agent is on the navmesh surface
	Offmesh       // Agent is traversing an offmesh connection
};

struct CNavAgentComponent
{
	dtPathCorridor       Corridor;
	dtNavMeshQuery*      pNavQuery = nullptr; //???need per-agent or can use pool in path queue or navmesh?
	PNavAgentSettings    Settings;
	CStrID               SettingsID;
	float                Radius = 0.3f;
	float                Height = 1.75f;
	vector3              TargetPos;
	dtPolyRef            TargetRef = 0;
	float                ReplanTime = 0.f;
	float                PathOptimizationTime = 0.f;
	U16                  AsyncTaskID = 0;
	ENavigationState     State = ENavigationState::Idle;
	ENavigationMode      Mode = ENavigationMode::Surface;
	U8                   CurrAreaType = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::AI::CNavAgentComponent>() { return "DEM::AI::CNavAgentComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::AI::CNavAgentComponent>()
{
	return std::make_tuple
	(
		Member(1, "Radius", &DEM::AI::CNavAgentComponent::Radius, &AI::CNavAgentComponent::Radius),
		Member(2, "Height", &DEM::AI::CNavAgentComponent::Height, &AI::CNavAgentComponent::Height),
		Member(3, "Settings", &DEM::AI::CNavAgentComponent::SettingsID, &AI::CNavAgentComponent::SettingsID)
	);
}

}
