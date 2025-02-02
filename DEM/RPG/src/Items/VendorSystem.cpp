#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Items/ItemContainerComponent.h>
#include <Items/VendorComponent.h>
#include <Items/ItemList.h>
#include <Items/ItemManager.h>

// A set of ECS systems required for vendor logic (shop item regeneration etc)

namespace DEM::RPG
{

void InitVendors(Game::CGameWorld& World, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CVendorComponent>([&ResMgr](auto EntityID, CVendorComponent& Component)
	{
		ResMgr.RegisterResource<CItemList>(Component.ItemGeneratorAsset);
		if (Component.ItemGeneratorAsset) Component.ItemGeneratorAsset->ValidateObject<CItemList>();
	});
}
//---------------------------------------------------------------------

// TODO: pass RNG or RNG seed in
void RegenerateGoods(Game::CGameSession& Session, Game::HEntity VendorID, bool Force)
{
	auto* pWorld = Session.FindFeature<DEM::Game::CGameWorld>();
	if (!pWorld) return;

	auto* pItemMgr = Session.FindFeature<DEM::RPG::CItemManager>();
	if (!pItemMgr) return;

	auto* pVendor = pWorld->FindComponent<DEM::RPG::CVendorComponent>(VendorID);
	if (!pVendor) return;

	auto* pItemList = pVendor->ItemGeneratorAsset->ValidateObject<DEM::RPG::CItemList>();
	if (!pItemList) return;

	// TODO: Session.FindFeature<DEM::PRG::CWorldCalendar>()->GetCurrentTimestamp()
	const U32 CurrTimestamp = 0;

	if (!Force)
	{
		if (pVendor->RegenerationPeriod)
		{
			if (pVendor->LastGenerationTimestamp + pVendor->RegenerationPeriod > CurrTimestamp) return;
		}
		else
		{
			// Single generation shop
			if (pVendor->LastGenerationTimestamp) return;
		}
	}

	auto* pContainer = pWorld->FindComponent<DEM::RPG::CItemContainerComponent>(pVendor->ContainerID ? pVendor->ContainerID : VendorID);
	if (!pContainer) return;

	// build a map of stacks to discard, or maybe a map of all stacks, then remove recreated from map, and then discard discardable stacks in map

	const auto* pVendorEntity = pWorld->GetEntity(VendorID);
	const auto LevelID = pVendorEntity ? pVendorEntity->LevelID : CStrID{};

	//!!!TODO: could get extended info from generator (tpl ID + entity ID + item component) for optimization!
	std::map<CStrID, U32> GeneratedItems;
	pItemList->Evaluate(Session, GeneratedItems);
	for (const auto [ItemID, Count] : GeneratedItems)
		pContainer->Items.push_back(pItemMgr->CreateStack(ItemID, Count, LevelID));

	pVendor->LastGenerationTimestamp = CurrTimestamp;
}
//---------------------------------------------------------------------

}
