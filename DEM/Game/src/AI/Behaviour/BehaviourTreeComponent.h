#pragma once
#include <AI/Behaviour/BehaviourTreePlayer.h>
#include <Resources/Resource.h>
#include <Data/Metadata.h>

// Character AI thinking component implemented using a behaviour tree

namespace DEM::AI
{

struct CBehaviourTreeComponent
{
	Resources::PResource Asset;
	CBehaviourTreePlayer Player;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CBehaviourTreeComponent>() { return "DEM::AI::CBehaviourTreeComponent"; }
template<> constexpr auto RegisterMembers<AI::CBehaviourTreeComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(AI::CBehaviourTreeComponent, Asset)
	);
}
static_assert(CMetadata<AI::CBehaviourTreeComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
