#pragma once
#include <Game/ECS/Entity.h>

// Utilities and algorithms for social dynamics and interactions

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::RPG
{

float GetDisposition(Game::CGameSession& Session, Game::HEntity NPCID);

}

