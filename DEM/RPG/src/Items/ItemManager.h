#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>

// Item manager keeps track of item templates and helps to manipulate item entities

// TODO: add temporary stack entity manipulation for the stack-on-cursor in the inventory UI.
// If not do that, each stack will take a new EntityID and will waste unique IDs without need.

namespace DEM::Game
{
	class CGameSession;
	class CGameWorld;
}

namespace Math
{
	class CTransformSRT;
}

namespace DEM::RPG
{

class CItemManager: public ::Core::CRTTIBaseClass
{
private:

	Game::CGameSession&                       _Session;
	std::unordered_map<CStrID, Game::HEntity> _Templates;

	Game::HEntity InternalCreateStack(Game::CGameWorld& World, CStrID LevelID, CStrID ItemID, U32 Count);

public:

	CItemManager(Game::CGameSession& Owner);

	void          GatherExistingTemplates();

	Game::HEntity CreateStack(CStrID ItemID, U32 Count, Game::HEntity Container);
	Game::HEntity CreateStack(CStrID ItemID, U32 Count, CStrID LevelID, const Math::CTransformSRT& WorldTfm);
};

}
