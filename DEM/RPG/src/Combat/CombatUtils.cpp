#include "CombatUtils.h"
#include <Combat/DestructibleComponent.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

void InflictDamage(Game::CGameWorld& World, Game::HEntity TargetID, CStrID Location, int Damage, EDamageType DamageType, Game::HEntity ActorID)
{
	auto pDestructible = World.FindComponent<CDestructibleComponent>(TargetID);
	if (!pDestructible || pDestructible->HP.GetFinalValue() <= 0) return;

	const auto& Absorption = pDestructible->DamageAbsorption.GetFinalValue();
	if (!Absorption.empty())
	{
		const auto It = Location ? Absorption.find(Location) : Absorption.cbegin();
		Damage -= It->second[static_cast<size_t>(DamageType)];
	}

	//!!!DBG TMP!
	Data::CData DmgTypeStr;
	DEM::ParamsFormat::Serialize(DmgTypeStr, DamageType);
	::Sys::DbgOut(("***DBG Hit: " + Game::EntityToString(ActorID) + " hits " + Game::EntityToString(TargetID) +
		" (" + std::to_string(pDestructible->HP.GetFinalValue()) + " HP)" +
		" for " + std::to_string(std::max(0, Damage)) + " HP (" + DmgTypeStr.GetValue<CString>().CStr() + ")\n").c_str());

	if (Damage <= 0) return;

	auto HP = pDestructible->GetHP();
	HP -= Damage;
	pDestructible->SetHP(HP);
	pDestructible->OnHit(Damage);

	if (pDestructible->HP.GetFinalValue() <= 0) pDestructible->OnDestroyed();
}
//---------------------------------------------------------------------

}
