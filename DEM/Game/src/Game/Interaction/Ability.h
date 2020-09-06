#pragma once
#include <Data/StringID.h>
#include <sol/sol.hpp>
#include <vector>
#include <set>

// Ability is a tool that describes a way to interact with the game. It is a context
// which helps to select the most appropriate action. Some abilities provide only a
// single action, some can provide lots of them for different targets.
// Examples are:
// - default interaction (talk with friendly NPC, attack hostile one, walk a navmesh)
// - character skills (mechanics can be used to repair broken automatons or disarm traps)
// - character abilities and spells (anything from sword attack to mass healing in area)
// - player interactions (explore object to read its textual description)
// - editor and cheat tools (select entity, teleport it to point)

namespace DEM::Game
{

struct CAbility
{
	std::string      IconID;
	std::string      Name;
	std::string      Description;
	sol::function    AvailabilityCondition;

	std::set<CStrID> Tags;

	std::vector<std::pair<CStrID, sol::function>> Interactions; // Interaction ID -> optional condition
};

}
