#include "CombatUtils.h"
#include <Combat/DestructibleComponent.h>
#include <Game/ECS/GameWorld.h>
#include <fmt/format.h>

namespace DEM::RPG
{

void InflictDamage(Game::CGameWorld& World, Game::HEntity TargetID, CStrID Location, int Damage, EDamageType DamageType, Game::HEntity ActorID)
{
	auto* pDestructible = World.FindComponent<CDestructibleComponent>(TargetID);
	if (!pDestructible || pDestructible->HP <= 0) return;

	float FinalDamage = static_cast<float>(Damage);

	const auto& Absorption = pDestructible->DamageAbsorption;
	if (!Absorption.empty())
	{
		const auto It = Absorption.find(Location);
		const auto& ZoneAbsorption = (It != Absorption.cend()) ? It->second : Absorption.cbegin()->second;
		FinalDamage -= ZoneAbsorption[static_cast<size_t>(DamageType)];
	}

	// TODO: resistance (%)
	// TODO: conditions here? Or outside? Like "apply poison damage if inflicted at least 1 physical damage"?

	// This RPG system operates on whole numbers for damage
	const auto FinalDamageInt = std::lround(FinalDamage);

	//!!!DBG TMP!
	//!!!FIXME: {fmt} doesn't see format_as(HEntity)!!!
	Data::CData DmgTypeStr;
	DEM::ParamsFormat::Serialize(DmgTypeStr, DamageType);
	::Sys::DbgOut("***DBG Hit: {} hits {} (was {} HP) for {} HP ({})\n"_format(
		Game::EntityToString(ActorID), Game::EntityToString(TargetID), pDestructible->HP, std::max(0L, FinalDamageInt), DmgTypeStr.GetValue<std::string>()));

	if (FinalDamageInt <= 0) return;

	pDestructible->HP -= FinalDamageInt;
	pDestructible->OnHit(FinalDamageInt);

	if (pDestructible->HP <= 0) pDestructible->OnDestroyed(DamageType);

	//!!!DBG TMP!
	if (pDestructible->HP <= 0) ::Sys::DbgOut("***DBG Destroyed: {}\n"_format(Game::EntityToString(TargetID)));
}
//---------------------------------------------------------------------

}
