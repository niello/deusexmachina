#pragma once
#include <Data/StringID.h>

// ECS systems and utils for character stat sheet

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{

void RemoveStatModifiers(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SourceID);
void InitStats(Game::CGameWorld& World, Game::CGameSession& Session, Resources::CResourceManager& ResMgr);
bool IsImmuneToAnyTag(const Game::CGameWorld& World, Game::HEntity EntityID, const std::set<CStrID>& Tags);

}
