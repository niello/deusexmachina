#include "CombatUtils.h"
#include <Combat/DestructibleComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scripting/Command.h>

namespace DEM::RPG
{

U32 InflictDamage(Game::CGameWorld& World, Game::HEntity TargetID, CStrID Location, int Damage, EDamageType DamageType, Game::HEntity ActorID)
{
	auto* pDestructible = World.FindComponent<CDestructibleComponent>(TargetID);
	if (!pDestructible || pDestructible->HP <= 0) return 0;

	float FinalDamage = static_cast<float>(Damage);

	const auto& Absorption = pDestructible->DamageAbsorption;
	if (!Absorption.empty() && IsAbsorbableDamageType(DamageType))
	{
		auto It = Absorption.find(Location);
		if (It == Absorption.cend())
		{
			It = Absorption.cbegin();
			Location = It->first;
		}
		FinalDamage -= It->second[static_cast<size_t>(DamageType)];
	}

	// TODO: resistance (%)
	// TODO: conditions here? Or outside? Like "apply poison damage if inflicted at least 1 physical damage"?

	// This RPG system operates on whole numbers for damage
	const auto FinalDamageInt = std::lround(FinalDamage);

	//!!!DBG TMP!
	//!!!FIXME: {fmt} doesn't see format_as(HEntity)!!!
	Data::CData DmgTypeStr;
	DEM::ParamsFormat::Serialize(DmgTypeStr, DamageType);
	::Sys::DbgOut("***DBG Hit: {} hits {} at {} (was {} HP) for {} HP ({})\n"_format(
		Game::EntityToString(ActorID), Game::EntityToString(TargetID), Location, pDestructible->HP, std::max(0L, FinalDamageInt), DmgTypeStr.GetValue<std::string>()));

	if (FinalDamageInt <= 0) return 0;

	pDestructible->HP -= FinalDamageInt;
	pDestructible->OnHit(FinalDamageInt);

	if (pDestructible->HP <= 0) pDestructible->OnDestroyed(DamageType);

	//!!!DBG TMP!
	if (pDestructible->HP <= 0) ::Sys::DbgOut("***DBG Destroyed: {}\n"_format(Game::EntityToString(TargetID)));

	return static_cast<U32>(FinalDamageInt);
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

// Can add params: Source(?), Hostile(? = true; or tag?)
bool Command_DealDamage(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars)
{
	if (!pParams || !pVars) return false;

	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// TODO: use ResolveEntityID? to allow direct ID.
	const auto TargetVarID = pParams->Get<CStrID>(CStrID("Target"), CStrID("Target"));
	const auto TargetEntityID = pVars->Get<Game::HEntity>(pVars->Find(TargetVarID), {});
	if (!TargetEntityID) return false;

	const auto SourceEntityID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceCreature")), {});

	const auto HitZone = pParams->Get<CStrID>(CStrID("HitZone"), CStrID::Empty);
	const auto Amount = std::lroundf(EvaluateCommandNumericValue(Session, pParams, pVars, CStrID("Amount"), 1.f));
	const auto DamageType = EvaluateCommandEnumValue(Session, pParams, CStrID("Type"), EDamageType::Raw);

	InflictDamage(*pWorld, TargetEntityID, HitZone, Amount, DamageType, SourceEntityID);

	return true;
}
//---------------------------------------------------------------------

}
