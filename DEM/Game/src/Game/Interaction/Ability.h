#pragma once
#include <Data/StringID.h>
//#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <vector>
#include <set>

// Ability is a tool that describes a way to interact with the game. It is kind of
// context which helps to select the most appropriate action. Some abilities provide
// only single action, some can provide lots of them for different targets.
// Examples are:
// - default interaction (talk with friendly NPC, attack hostile one, walk a navmesh)
// - character skills (mechanics can be used to repair broken automatons or disarm traps)
// - character abilities and spells (anything from sword attack to mass healing in area)
// - player interactions (explore object to read its textual description)
// - editor and cheat tools (select entity, teleport it to point)

namespace Data
{
	class CParams;
}

namespace DEM::Game
{

//???make it struct accessible only from the CInteractionManager? With no logic, all logic there too,
// fed by CInteractionContext objects. Or contexts use manager and implement logic?
class CAbility
{
protected:

	std::string         IconID;
	std::string         Name;
	std::string         Description;

	std::vector<CStrID> Actions;
	std::set<CStrID>    Tags;

	// scripted condition IsAvailableFor(SelectedActors)
	//???load a chunk of code as lua function? need context, not only SelectedActors.
	//For example, if function is CheckAllHaveSkill(SelectedActors, "Mechanics")
	//Here we don't know about the second arg.
	// Add "local SelectedTargets = ...; return " to the condition and it is set for sol3
	// Store as sol::load_result/protected_function, not as string. Can load precompiled, if ability desc is compiled to PRM
	sol::protected_function Condition;

public:

	static CAbility CreateFromParams(const Data::CParams& Params);

	//???need something specific for switchable abilities (on-off) or is controlled from outside?
};

}
