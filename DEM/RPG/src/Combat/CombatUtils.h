#pragma once
#include <Combat/Damage.h>
#include <Game/ECS/Entity.h>
#include <Game/GameVarStorage.h>

// Utilities and algorithms for combat, attacks and damage

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{

U32 InflictDamage(Game::CGameWorld& World, Game::HEntity TargetID, CStrID Location, int Damage, EDamageType DamageType, Game::HEntity ActorID = {});
void ApplyArmorModifiers(Game::CGameWorld& World, Game::HEntity TargetID, const std::map<CStrID, CZoneDamageAbsorptionMod>& ZoneMods, CStrID SourceID);
void RemoveArmorModifiers(Game::CGameWorld& World, Game::HEntity TargetID, const std::map<CStrID, CZoneDamageAbsorptionMod>& ZoneMods, CStrID SourceID);
bool Command_DealDamage(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars);

}

