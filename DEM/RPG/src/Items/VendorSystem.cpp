#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Items/ItemComponent.h>
#include <Items/ItemStackComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Items/VendorComponent.h>
#include <Items/ItemList.h>
#include <Items/ItemManager.h>
#include <Items/ItemUtils.h>

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

// TODO: pass RNG instance or RNG seed in
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

	const auto* pVendorEntity = pWorld->GetEntity(VendorID);
	const auto LevelID = pVendorEntity ? pVendorEntity->LevelID : CStrID{};

	auto* pContainer = pWorld->FindComponent<DEM::RPG::CItemContainerComponent>(pVendor->ContainerID ? pVendor->ContainerID : VendorID);
	if (!pContainer) return;

	// Gather current stacks available for replacing
	std::map<const CItemComponent*, std::vector<std::pair<size_t, CItemStackComponent*>>> CurrReplaceableStacks;
	for (size_t SlotIdx = 0; SlotIdx < pContainer->Items.size(); ++SlotIdx)
	{
		const auto StackID = pContainer->Items[SlotIdx];
		if (pWorld->FindComponent<const CGeneratedComponent>(StackID))
			if (auto* pStack = pWorld->FindComponent<CItemStackComponent>(StackID))
				if (const auto* pProto = pWorld->FindComponent<const CItemComponent>(pStack->Prototype))
					CurrReplaceableStacks[pProto].push_back(std::make_pair(SlotIdx, pStack));
	}

	// Generate and add new items
	std::map<CStrID, CItemStackData> GeneratedItems;
	pItemList->Evaluate(Session, GeneratedItems);
	for (auto& [ItemID, Data] : GeneratedItems)
	{
		// Try to reuse an existing generated stack for new generated items
		auto ItCurr = CurrReplaceableStacks.find(Data.pItem);
		if (ItCurr != CurrReplaceableStacks.cend())
		{
			auto& Stacks = ItCurr->second;
			for (auto ItStack = Stacks.begin(); ItStack != Stacks.end(); ++ItStack)
			{
				auto* pStack = ItStack->second;
				if (CanMergeItems(Data.EntityID, pStack))
				{
					// Remove previous items, add new ones
					pStack->Count = Data.Count;
					Data.Count = 0;
					Stacks.erase(ItStack);
					break;
				}
			}
		}

		// If not reused, create a new generated stack
		if (Data.Count)
		{
			const auto NewStackID = pItemMgr->CreateStack(ItemID, Data.Count, LevelID);
			pWorld->AddComponent<CGeneratedComponent>(NewStackID);
			pContainer->Items.push_back(NewStackID);
		}
	}

	// Discard remaining previously generated items
	for (auto& [pItem, Stacks] : CurrReplaceableStacks)
	{
		for (const auto [SlotIdx, pStack] : Stacks)
		{
			pWorld->DeleteEntity(pContainer->Items[SlotIdx]);
			pContainer->Items[SlotIdx] = {};
		}
	}

	RemoveEmptySlots(pContainer->Items);

	pVendor->LastGenerationTimestamp = CurrTimestamp;
}
//---------------------------------------------------------------------

}
