#pragma once
#include <Data/StringID.h>
#include <vector>

// Ability is a tool that describes a way to interact with the game. It is kind of
// context which helps to select the most appropriate action. Some abilities provide
// only single action, some can provide lots of them for different targets.
// Examples are:
// - default interaction (talk with friendly NPC, attack hostile one, walk a navmesh)
// - character skills (mechanics can be used to repair broken automatons or disarm traps)
// - abilities and spells (anything from sword attack to mass healing in area)
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

	std::vector<CStrID> Actions;

public:

	static CAbility CreateFromParams(const Data::CParams& Params);

	CStrID ChooseAction() const;

	//???need something specific for switchable abilities (on-off) or is controlled from outside?

	// rendering info? icon, cursor
};

}
