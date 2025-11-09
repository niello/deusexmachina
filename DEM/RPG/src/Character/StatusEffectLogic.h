#pragma once
#include <Game/GameVarStorage.h>

// ECS systems and utils for character status effect application and management

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{
class CStatusEffectData;
struct CStatusEffectInstance;
struct CStatusEffectStack;

bool Command_ApplyStatusEffect(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars);
bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, CStatusEffectInstance&& Instance);
void TriggerStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity EntityID, CStrID Event, Game::CGameVarStorage* pVars);
void TriggerStatusEffect(Game::CGameSession& Session, const CStatusEffectStack& Stack, CStrID Event, Game::CGameVarStorage* pVars);
void UpdateStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, float dt);

}
