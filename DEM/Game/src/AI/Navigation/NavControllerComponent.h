#pragma once
#include <AI/Navigation/TraversalAction.h>
//#include <Game/ECS/ComponentStorage.h>
//#include <Data/Ptr.h>
#include <Data/Metadata.h>
#include <Data/StringID.h>
#include <Core/Factory.h>
//#include <Events/EventNative.h>
//#include <Math/Vector3.h>
//#include <DetourPathCorridor.h>

// Navigation controller component allows an entity to influence navigation rules in a region

namespace DEM::AI
{

struct CNavControllerComponent
{
	CStrID                    RegionID;
	DEM::AI::PTraversalAction Action;

	void SetActionID(const std::string& ActionID)
	{
		Action = DEM::Core::CFactory::Instance().Create<DEM::AI::CTraversalAction>(ActionID.c_str());
	}

	const std::string& GetActionID() const
	{
		static const std::string EmptyString;
		return Action ? Action->GetClassName() : EmptyString;
	}
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::AI::CNavControllerComponent>() { return "DEM::AI::CNavControllerComponent"; }
template<> constexpr auto RegisterMembers<DEM::AI::CNavControllerComponent>()
{
	return std::make_tuple
	(
		Member(1, "RegionID", &DEM::AI::CNavControllerComponent::RegionID, &AI::CNavControllerComponent::RegionID),
		Member<DEM::AI::CNavControllerComponent, std::string>(2, "ActionID", &DEM::AI::CNavControllerComponent::GetActionID, &AI::CNavControllerComponent::SetActionID)
	);
}

}
