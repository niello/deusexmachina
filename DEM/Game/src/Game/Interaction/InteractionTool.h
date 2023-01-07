#pragma once
#include <Data/StringID.h>
#include <Scripting/SolGame.h>
#include <vector>
#include <set>

// A tool that describes how player input converts into interaction with the game.
// Some tools may trigger only a single ability, others can vary based on target and preconditions.
// Examples are:
// - default interaction (talk with friendly NPC, attack hostile one, walk a navmesh, acivate objects)
// - character skills (mechanics can be used to repair broken automatons or disarm traps)
// - character abilities and spells (anything from sword attack to mass healing in area)
// - player interactions (select actor, examine object)
// - editor and cheat tools (select entity, teleport it to point)

namespace DEM::Game
{

struct CInteractionTool
{
	std::string      IconID;
	std::string      Name;
	std::string      Description;

	//std::set<CStrID> Tags; // FIXME: tags not here, in actor abilities?!

	std::vector<std::pair<CStrID, sol::function>> Interactions; // Interaction ID -> optional condition
};

}
