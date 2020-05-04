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

	//!!!can instead be universal ITarget, with impls like "point", "entity", "nearest ally" etc!
	//???or navigation is always to point, and point is updated from target externally? strange.
	vector3 _Destination;

	Navigate(const vector3& Destination) : _Destination(Destination) {}
};

struct CNavigationComponent
{
	//dtPathCorridor Corridor;
	int x;

	//???what is here, what is in request?

	//???navmesh query - here or in request?

	//???need at all, or navigation system must decompose Navigate action from DEM::AI::CActionQueue component?
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
