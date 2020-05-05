#pragma once
#include <Data/Metadata.h>
#include <Events/EventNative.h>
#include <Math/Vector3.h>
#include <DetourPathCorridor.h>

// Navigation component allows an entity to plan path along navigation mesh

namespace DEM::AI
{

class Navigate: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	//!!!_Destination can instead be universal ITarget, with impls like "point", "entity", "nearest ally" etc!
	//???or navigation is always to point, and point is updated from target externally? strange.
	vector3 _Destination;
	float   _Speed = 0.f;
	// MinDistance, MaxDistance

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

struct CNavigationComponent
{
	dtPathCorridor       Corridor;
	dtNavMeshQuery*      pNavQuery = nullptr;
	const dtQueryFilter* pNavFilter = nullptr;
	vector3              Destination;
	ENavigationState     State = ENavigationState::Idle;
	ENavigationMode      Mode = ENavigationMode::Surface;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::AI::CNavigationComponent>() { return "DEM::AI::CNavigationComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::AI::CNavigationComponent>()
{
	return std::make_tuple
	(
		//Member(1, "ClipID", &DEM::AI::CNavigationComponent::ClipID, &AI::CNavigationComponent::ClipID),
	);
}

}
