#include "StatusEffectLogic.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

bool AddStatusEffect(Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, const CStatusEffectInstance& Instance)
{
	if (Instance.Magnitude <= 0.f) return false;

	auto* pStatusEffectComponent = World.FindOrAddComponent<CStatusEffectsComponent>(TargetID);
	if (!pStatusEffectComponent) return false;

	// check by tags if this effect is blocked

	auto& Stack = pStatusEffectComponent->StatusEffectStacks[Effect.ID];

	// try to merge with policy

	// if not merged, add to instance list (stack must update totals)
	// subscribe on expiration condition events, set magnitude 0 (stack must update totals)

	::Sys::DbgOut("Status effect: {}\n"_format(Effect.ID));
	NOT_IMPLEMENTED;

	return false;
}
//---------------------------------------------------------------------

}
