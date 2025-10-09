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
		if (IsAbsorbableDamageType(DamageType))
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

void ApplyArmorModifiers(Game::CGameWorld& World, Game::HEntity TargetID, const std::map<CStrID, CZoneDamageAbsorptionMod>& ZoneMods, CStrID SourceID)
{
	auto* pDestructible = World.FindComponent<CDestructibleComponent>(TargetID);
	if (!pDestructible) return;

	// TODO: to some centralized list
	constexpr U16 ModifierPriority_Equipment = 0;

	// TODO PERF: can sync linearly, both maps are sorted by a hit zone
	for (const auto& [Zone, Mods] : ZoneMods)
	{
		// Don't create new protected zones by the armor if a creature doesn't have them naturally
		auto It = pDestructible->DamageAbsorption.find(Zone);
		if (It == pDestructible->DamageAbsorption.cend()) continue;

		for (size_t i = 0; i < Mods.size(); ++i)
			if (Mods[i])
				It->second[i].AddModifier(EModifierType::Add, static_cast<float>(Mods[i]), SourceID, ModifierPriority_Equipment);
	}
}
//---------------------------------------------------------------------

void RemoveArmorModifiers(Game::CGameWorld& World, Game::HEntity TargetID, const std::map<CStrID, CZoneDamageAbsorptionMod>& ZoneMods, CStrID SourceID)
{
	auto* pDestructible = World.FindComponent<CDestructibleComponent>(TargetID);
	if (!pDestructible) return;

	// TODO PERF: can sync linearly, both maps are sorted by a hit zone
	for (const auto& [Zone, Mods] : ZoneMods)
	{
		auto It = pDestructible->DamageAbsorption.find(Zone);
		if (It == pDestructible->DamageAbsorption.cend()) continue;

		for (auto& Stat : It->second)
			Stat.RemoveModifiers(SourceID);
	}
}
//---------------------------------------------------------------------

}
