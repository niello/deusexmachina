#pragma once
#include <Game/ECS/Entity.h>

// Item stack consists of some count of exactly equal items.
// Stack can reference a prototype entity to avoid cloning item components per stack.
// Components different from the prototype are attached to the stack entity. A stack
// is detached from a prototype only when some prototype components are removed from it.

namespace DEM::RPG
{

struct CItemStackComponent
{
	Game::HEntity Prototype;
	U32           Count = 1;
	U16           Modified = 0; // To optimize out deep comparison when stacking
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<RPG::CItemStackComponent>() { return "DEM::RPG::CItemStackComponent"; }
template<> inline constexpr auto RegisterMembers<RPG::CItemStackComponent>()
{
	return std::make_tuple
	(
		Member(1, "Count", &RPG::CItemStackComponent::Count, &RPG::CItemStackComponent::Count)
	);
}

}
