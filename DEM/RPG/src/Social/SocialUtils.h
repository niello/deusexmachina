#pragma once
#include <Game/ECS/Entity.h>

// Utilities and algorithms for social dynamics and interactions

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::RPG
{
constexpr float MAX_DISPOSITION = 100.f;

float GetDisposition(const Game::CGameSession& Session, Game::HEntity FromCharacter, Game::HEntity ToCharacter);
bool  IsInFaction(const Game::CGameSession& Session, Game::HEntity CharacterID, CStrID FactionID);
bool  IsPartyMember(const Game::CGameSession& Session, Game::HEntity CharacterID);

}

