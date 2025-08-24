#include "CharacterSheet.h"
#include <Character/StatsComponent.h>
#include <Character/Archetype.h>
#include <Combat/DestructibleComponent.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

void CCharacterSheet::RegisterScriptAPI(sol::state& State)
{
	auto UT = State.new_usertype<CCharacterSheet>("CCharacterSheet");

	// Pass a final value of the stat to Lua, lazy-evaluating if necessary
	UT.set_function(sol::meta_function::index, [](CCharacterSheet& Self, std::string_view Key, sol::this_state s)
	{
		auto Var = Self.FindStatAndRegisterAccess(Key);

		if (const auto* ppStat = std::get_if<CNumericStat*>(&Var))
			return sol::make_object(s, (*ppStat)->Get());

		if (const auto* ppStat = std::get_if<CBoolStat*>(&Var))
			return sol::make_object(s, (*ppStat)->Get());

		return sol::object();
	});

	UT.set_function(sol::meta_function::new_index, [](CCharacterSheet&, std::string_view, sol::stack_object)
	{
		::Sys::Error("CCharacterSheet is read-only!");
	});
}
//---------------------------------------------------------------------

CCharacterSheet::CCharacterSheet(Game::CGameWorld& World, Game::HEntity EntityID)
{
	if (auto* pComponent = World.FindComponent<Sh2::CStatsComponent>(EntityID))
		CollectStatsFromComponent(pComponent);

	if (auto* pComponent = World.FindComponent<RPG::CDestructibleComponent>(EntityID))
		CollectStatsFromComponent(pComponent);

	// TODO: subscribe on each component's addition if it is missing and on removal if it is present
}
//---------------------------------------------------------------------

CCharacterSheet::TStat CCharacterSheet::FindStat(std::string_view Name) const
{
	auto It = _Stats.find(Name);
	return (It != _Stats.cend()) ? It->second.Stat : std::monostate{};
}
//---------------------------------------------------------------------

CCharacterSheet::TStat CCharacterSheet::FindStatAndRegisterAccess(std::string_view Name)
{
	auto It = _Stats.find(Name);
	if (It == _Stats.cend()) return std::monostate{};
	It->second.Accessed = true;
	return It->second.Stat;
}
//---------------------------------------------------------------------

}
