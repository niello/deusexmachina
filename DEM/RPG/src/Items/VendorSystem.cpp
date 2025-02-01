#include <Game/ECS/GameWorld.h>
#include <Items/VendorComponent.h>
#include <Items/ItemList.h>

// A set of ECS systems required for vendor logic (shop item regeneration etc)

namespace DEM::RPG
{

void InitVendors(Game::CGameWorld& World, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CVendorComponent>([&ResMgr](auto EntityID, CVendorComponent& Component)
	{
		ResMgr.RegisterResource<CItemList>(Component.ItemGeneratorAsset);
		if (!Component.ItemGeneratorAsset) return;

		if (auto* pItemList = Component.ItemGeneratorAsset->ValidateObject<CItemList>())
		{
			// ... generate now if was never generated? or wait for the first access?
			//pItemList->Evaluate(Session, Out);
		}
	});
}
//---------------------------------------------------------------------

}
