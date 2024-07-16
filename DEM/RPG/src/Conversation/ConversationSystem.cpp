#include <Game/ECS/GameWorld.h>
#include <Conversation/TalkingComponent.h>
#include <Scripting/Flow/FlowAsset.h>

namespace DEM::RPG
{

void InitTalkable(Game::CGameWorld& World, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<DEM::RPG::CTalkingComponent>([&ResMgr](auto EntityID, DEM::RPG::CTalkingComponent& Component)
	{
		ResMgr.RegisterResource<DEM::Flow::CFlowAsset>(Component.Asset);
		if (!Component.Asset) return;

		if (auto pFlow = Component.Asset->ValidateObject<DEM::Flow::CFlowAsset>())
		{
			// ...
		}
	});
}
//---------------------------------------------------------------------

}
