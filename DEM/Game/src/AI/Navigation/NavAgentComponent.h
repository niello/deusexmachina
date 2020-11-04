#pragma once
#include <AI/Navigation/NavAgentSettings.h>
#include <AI/Navigation/NavMap.h>
#include <Game/ECS/ComponentStorage.h>
#include <Data/Ptr.h>
#include <Data/Metadata.h>
#include <Events/EventNative.h>
#include <Math/Vector3.h>
#include <DetourPathCorridor.h>

// Navigation agent component allows an entity to plan path along navigation mesh

namespace DEM::AI
{

class Navigate: public Events::CEventNative
{
	NATIVE_EVENT_DECL(Navigate, Events::CEventNative);

public:

	vector3 _Destination;
	vector3 _FinalFacing;
	float   _Speed = 0.f;

	explicit Navigate(const vector3& Destination, float Speed) : _Destination(Destination), _Speed(Speed) {}
	explicit Navigate(const vector3& Destination, const vector3& FinalFacing, float Speed) : _Destination(Destination), _FinalFacing(FinalFacing), _Speed(Speed) {}
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
	Recovery = 0, // Agent is outside the navmesh and will recover to the surface unless Idle
	Surface,      // Agent is on the navmesh surface
	Offmesh       // Agent is traversing an offmesh connection
};

struct CNavAgentComponent
{
	dtPathCorridor       Corridor;
	dtNavMeshQuery*      pNavQuery = nullptr; //???need per-agent or can use pool in path queue or navmesh?
	PNavMap              NavMap;
	PNavAgentSettings    Settings;
	CStrID               SettingsID;
	float                Radius = 0.3f;
	float                Height = 1.75f;
	vector3              TargetPos;
	dtPolyRef            TargetRef = 0;
	dtPolyRef            OffmeshRef = 0; // Currently triggered offmesh connection. Traversal may not yet started.
	float                ReplanTime = 0.f;
	float                PathOptimizationTime = 0.f;
	U16                  AsyncTaskID = 0;
	//U16                  PathVersion = 0; // Increments each time the path corridor is replanned or optimized. Each new path begins from zero.
	ENavigationState     State = ENavigationState::Idle;
	ENavigationMode      Mode = ENavigationMode::Surface; //!!!if will store offmesh ref not in corridor, bool Valid will be enough instead!
	U8                   CurrAreaType = 0;
	bool                 IsTraversingLastEdge = false; // FIXME: can rewrite better?
};

inline bool InRange(const float* v0, const float* v1, float AgentHeight, float SqRange)
{
	const float dx = v1[0] - v0[0];
	const float dz = v1[2] - v0[2];
	return (dx * dx + dz * dz) <= SqRange && std::abs(v0[1] - v1[1]) < AgentHeight;
}
//---------------------------------------------------------------------

}

namespace DEM::Game
{

template<>
struct TComponentTraits<DEM::AI::CNavAgentComponent>
{
	using TStorage = CSparseComponentStorage<DEM::AI::CNavAgentComponent, true>;
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
