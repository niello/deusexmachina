#pragma once
#include <Game/GameVarStorage.h>

// ECS systems and utils for character stat sheet

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace Resources
{
	class CResourceManager;
}

namespace DEM::RPG
{

void RemoveStatModifiers(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SourceID);
void InitStats(Game::CGameWorld& World, Game::CGameSession& Session, Resources::CResourceManager& ResMgr);
bool Command_ModifyStat(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars);

}
