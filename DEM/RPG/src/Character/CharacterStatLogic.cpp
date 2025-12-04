#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Character/Archetype.h>
#include <Character/StatsComponent.h>
#include <Combat/DestructibleComponent.h>

//???where to cache Lua sheet interface? or recreate each time a secondary stat formula is recalculated? maps then will not speed things up!
//if not cacheable, must be a simple set of pointers to components, with O(n) direct string comparison in __index. Will then use some
//utility functions like CNumericStat* FindStat(component, string), maybe using metadata.
//???store sheet interface in CStatsComponent, reuse at each formula evaluation! if components like destructible can change in runtime, must listen add/remove!
//!!!adding components will also need an archetype to be applied to its stats!

namespace DEM::RPG
{

void RemoveStatModifiers(Game::CGameWorld& World, Game::HEntity EntityID, CStrID SourceID)
{
	//???!!!how to optimize scanning all stats? store SourceID->vector<StatID> index map? or subscribe character on modifier source lifetime events?!
	if (auto* pStats = World.FindComponent<Sh2::CStatsComponent>(EntityID))
	{
		DEM::Meta::CMetadata<Sh2::CStatsComponent>::ForEachMember([pStats, SourceID](const auto& Member)
		{
			if constexpr (std::is_same_v<DEM::Meta::TMemberValue<decltype(Member)>, CNumericStat>)
				Member.GetValueRef(*pStats).RemoveModifiers(SourceID);
			else if constexpr (std::is_same_v<DEM::Meta::TMemberValue<decltype(Member)>, CBoolStat>)
				Member.GetValueRef(*pStats).RemoveModifiers(SourceID);
		});

		//!!!FIXME: improve handling code! Now hard to maintain and understand!
		for (auto& Stat : pStats->TagImmunity)
			Stat.second.RemoveModifiers(SourceID);
	}
}
//---------------------------------------------------------------------

void InitStats(Game::CGameWorld& World, Game::CGameSession& Session, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<Sh2::CStatsComponent>([&ResMgr, &World, &Session](auto EntityID, Sh2::CStatsComponent& Stats)
	{
		ResMgr.RegisterResource<CArchetype>(Stats.Archetype);
		if (!Stats.Archetype) return;

		if (auto* pArchetype = Stats.Archetype->ValidateObject<CArchetype>())
		{
			//!!!TODO: need pArchetype fallback to parent (base)! better do on load, then need Ptr instead of unique_ptr for descs.

			//!!!FIXME: not stable enough! easy to forget a new field!
			Stats.Strength.SetDesc(pArchetype->Strength.get());
			Stats.Constitution.SetDesc(pArchetype->Constitution.get());
			Stats.Dexterity.SetDesc(pArchetype->Dexterity.get());
			Stats.Perception.SetDesc(pArchetype->Perception.get());
			Stats.Erudition.SetDesc(pArchetype->Erudition.get());
			Stats.Learnability.SetDesc(pArchetype->Learnability.get());
			Stats.Charisma.SetDesc(pArchetype->Charisma.get());
			Stats.Willpower.SetDesc(pArchetype->Willpower.get());

			//???MaxHP in destructible or in sheet? destructible could store only current HP, e.g. door needs no max HP?! what about repairing?
			//healing and repairing try to access the sheet? then the door can be repaired indefinitely because max HP is unknown?
			Stats.MaxHP.SetDesc(pArchetype->MaxHP.get());

			Stats.CanMove.SetDesc(pArchetype->CanMove.get());
			Stats.CanInteract.SetDesc(pArchetype->CanInteract.get());
			Stats.CanSpeak.SetDesc(pArchetype->CanSpeak.get());

			for (const CStrID Tag : pArchetype->TagImmunity)
				Stats.TagImmunity[Tag].SetBaseValue(true);

			//!!!must init other components!

			if (auto* pDestructible = World.FindComponent<CDestructibleComponent>(EntityID))
			{
				// Logically this check is needed because damage absorption is a primary stat.
				// Technically - because this stat isn't inited with the sheet (at least for now).
				n_assert_dbg(!pArchetype->DamageAbsorption || !pArchetype->DamageAbsorption->Formula);

				for (const CStrID HitZone : pArchetype->HitZones)
				{
					// Add a hit zone with its absorption values
					auto& ZoneAbsorption = pDestructible->DamageAbsorption[HitZone];

					// Initialize all absorption stats from the same definition for now. Can change later.
					if (pArchetype->DamageAbsorption)
						for (auto& Stat : ZoneAbsorption)
							Stat.SetDesc(pArchetype->DamageAbsorption.get());
				}
			}

			// NB: now must do this after setting descs
			//???what about runtime changes?
			Stats.Sheet = new CCharacterSheet(World, EntityID);
		}
	});
}
//---------------------------------------------------------------------

}
