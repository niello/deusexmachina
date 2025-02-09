#include "VendorLogic.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Items/ItemComponent.h>
#include <Items/ItemStackComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Items/VendorComponent.h>
#include <Items/ItemList.h>
#include <Items/ItemManager.h>
#include <Items/ItemUtils.h>
#include <Social/SocialUtils.h>
#include <Character/SocialComponent.h>
#include <Character/SkillsComponent.h> // TODO: or use a skill check utility!

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

void GetItemPrices(Game::CGameSession& Session, Game::HEntity VendorID, Game::HEntity BuyerID, const std::set<Game::HEntity>& ItemIDs, std::map<Game::HEntity, CVendorCoeffs>& Out)
{
	auto* pWorld = Session.FindFeature<DEM::Game::CGameWorld>();
	if (!pWorld) return;

	auto* pVendor = pWorld->FindComponent<DEM::RPG::CVendorComponent>(VendorID);
	if (!pVendor) return;

	// TODO: to global balance
	constexpr float DefaultBuyFromVendorCoeff = 2.0f;
	constexpr float DefaultSellToVendorCoeff = 0.5f;
	constexpr float MaxDispositionPriceChange = 0.25f;

	float BuyFromVendorCoeff = pVendor->BuyFromVendorCoeff.value_or(DefaultBuyFromVendorCoeff);
	float SellToVendorCoeff = pVendor->SellToVendorCoeff.value_or(DefaultSellToVendorCoeff);

	// Modify prices from disposition
	const float Disposition = GetDisposition(Session, VendorID, BuyerID);
	const float DispositionPriceChange = MaxDispositionPriceChange * Disposition / MAX_DISPOSITION;
	BuyFromVendorCoeff -= DispositionPriceChange;
	SellToVendorCoeff += DispositionPriceChange;

	// Diplomacy / haggle skill
	auto* pVendorSkills = pWorld->FindComponent<const Sh2::CSkillsComponent>(VendorID);
	auto* pBuyerSkills = pWorld->FindComponent<const Sh2::CSkillsComponent>(BuyerID);
	// SkillValue = pSkills ? pSkills->SkillValue : 0
	// then do an opposed roll (what to do if skill is 0 i.e. not opened?)
	// ...

	// Protect game economy from money farming
	if (BuyFromVendorCoeff < SellToVendorCoeff)
	{
		BuyFromVendorCoeff = (BuyFromVendorCoeff + SellToVendorCoeff) * 0.5f;
		SellToVendorCoeff = BuyFromVendorCoeff;
	}

	// Get overrides from the vendor script
	if (auto ScriptObject = Session.GetScript(pVendor->ScriptAssetID))
	{
		// pass VendorID, BuyerID, ItemIDs and calculated default BuyFromVendorCoeff and SellToVendorCoeff
		// get overrides as a map (item tpl Game::HEntity or CStrID -> [BuyFromVendorCoeff, SellToVendorCoeff])
		// if has empty key, it is override for default BuyFromVendorCoeff and SellToVendorCoeff
	}

	// Get overrides from vendor factions for items not overridden by the vendor itself
	if (auto* pVendorSocial = pWorld->FindComponent<const CSocialComponent>(VendorID))
	{
		for (const auto FactionID : pVendorSocial->Factions)
		{
			// same as with ScriptObject above
			// for the faction currency must make BuyFromVendorCoeff == SellToVendorCoeff so that std::round(ItemPrice * Coeff) = DesiredPrice
		}
	}

	Out.emplace(Game::HEntity{}, CVendorCoeffs{ BuyFromVendorCoeff, SellToVendorCoeff });
}
//---------------------------------------------------------------------

CItemPrices GetItemStackPrices(Game::CGameSession& Session, Game::HEntity StackID, const CVendorCoeffs& Coeffs)
{
	auto* pWorld = Session.FindFeature<DEM::Game::CGameWorld>();
	if (!pWorld) return {};

	auto* pItem = FindItemComponent<const CItemComponent>(*pWorld, StackID);
	if (!pItem) return {};

	// Item instance modifiers:
	// - unidentified and misidentified item coeffs
	// - charges (essential like in wands or replenishable like bullets)
	// - item HP
	// Coeffs for this must be requested from the balance.

	const float BuyPrice = pItem->Price * Coeffs.BuyFromVendorCoeff;
	const float SellPrice = pItem->Price * Coeffs.SellToVendorCoeff;

	CItemPrices Prices;

	if (BuyPrice <= 0.f)
	{
		Prices.BuyFromVendorPrice = 0;
		Prices.BuyFromVendorQuantity = 0;
	}
	else if (BuyPrice >= 1.f)
	{
		Prices.BuyFromVendorPrice = static_cast<U32>(std::round(BuyPrice));
		Prices.BuyFromVendorQuantity = 1;
	}
	else
	{
		Prices.BuyFromVendorPrice = 1;
		Prices.BuyFromVendorQuantity = static_cast<U32>(std::ceil(1.f / BuyPrice));
	}

	if (SellPrice <= 0.f)
	{
		Prices.SellToVendorPrice = 0;
		Prices.SellToVendorQuantity = 0;
	}
	else if (SellPrice >= 1.f)
	{
		Prices.SellToVendorPrice = static_cast<U32>(std::round(SellPrice));
		Prices.SellToVendorQuantity = 1;
	}
	else
	{
		Prices.SellToVendorPrice = 1;
		Prices.SellToVendorQuantity = static_cast<U32>(std::ceil(1.f / SellPrice));
	}

	return Prices;
}
//---------------------------------------------------------------------

}
