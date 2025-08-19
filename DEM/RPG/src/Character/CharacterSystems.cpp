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
			// ... init stats from archetype ...
			// can cache sheet interface at the same time?

			//!!!TODO: need pArchetype fallback to parent (base)!

			//???use reflection to iterate over stats without boilerplate?

			//!!!without archetype there can be no formulas, sheet may be not needed. But also might save it in CStatsComponent just in case.
			//strong refs will be in each found CNumericStat anyway

			//???pass Lua state to archetype loader as a constructor argument? or even session!

			Stats.Strength.SetDesc(pArchetype->Strength.get());

			Stats.CanSpeak.SetDesc(pArchetype->CanSpeak.get());
		}
	});
}
//---------------------------------------------------------------------

}
