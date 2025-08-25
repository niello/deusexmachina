#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Character/Archetype.h>
#include <Character/StatsComponent.h>

namespace DEM::RPG
{

//???where to cache Lua sheet interface? or recreate each time a secondary stat formula is recalculated? maps then will not speed things up!
//if not cacheable, must be a simple set of pointers to components, with O(n) direct string comparison in __index. Will then use some
//utility functions like CNumericStat* FindStat(component, string), maybe using metadata.
//???store sheet interface in CStatsComponent, reuse at each formula evaluation! if components like destructible can change in runtime, must listen add/remove!
//!!!adding components will also need an archetype to be applied to its stats!

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

			Stats.MaxHP.SetDesc(pArchetype->MaxHP.get());

			Stats.CanMove.SetDesc(pArchetype->CanMove.get());
			Stats.CanInteract.SetDesc(pArchetype->CanInteract.get());
			Stats.CanSpeak.SetDesc(pArchetype->CanSpeak.get());

			//!!!must init other components!
			//???MaxHP in destructible or in sheet? destructible could store only current HP, e.g. door needs no max HP?! what about repairing?

			// NB: now must do this after setting descs
			//???what about runtime changes?
			Stats.Sheet = new CCharacterSheet(World, EntityID);

			//!!!DBG TMP!
			::Sys::Log("***DBG MaxHP = {}"_format(Stats.MaxHP.Get()));
		}
	});
}
//---------------------------------------------------------------------

}
