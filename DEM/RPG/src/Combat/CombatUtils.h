#pragma once
#include <Combat/Damage.h>
#include <Game/ECS/Entity.h>

// Utilities and algorithms for combat, attacks and damage

namespace DEM::Game
{
	class CGameWorld;
}

namespace DEM::RPG
{

void InflictDamage(Game::CGameWorld& World, Game::HEntity TargetID, CStrID Location, int Damage, EDamageType DamageType, Game::HEntity ActorID = {});

}

