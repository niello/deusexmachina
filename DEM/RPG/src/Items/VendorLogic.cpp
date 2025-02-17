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
#include <Social/SocialManager.h>
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

		ResMgr.RegisterResource<CItemList>(Component.CurrencyListAsset);
		if (Component.CurrencyListAsset) Component.CurrencyListAsset->ValidateObject<CItemList>();
	});
}
//---------------------------------------------------------------------

// TODO: pass RNG instance or RNG seed in
void RegenerateGoods(Game::CGameSession& Session, Game::HEntity VendorID, bool Force)
{
	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto* pItemMgr = Session.FindFeature<CItemManager>();
	if (!pItemMgr) return;

	auto* pVendor = pWorld->FindComponent<CVendorComponent>(VendorID);
	if (!pVendor) return;

	// TODO: Session.FindFeature<PRG::CWorldCalendar>()->GetCurrentTimestamp()
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

	// Regenerate money
	U32 PrevMoneyRemaining = 0;
	if (pVendor->Money > pVendor->MaxGeneratedMoney)
		PrevMoneyRemaining = static_cast<U32>(Math::RandomFloat(0.3f, 0.8f) * (pVendor->Money - pVendor->MaxGeneratedMoney));
	pVendor->Money = Math::RandomU32(pVendor->MinGeneratedMoney, pVendor->MaxGeneratedMoney) + PrevMoneyRemaining;

	// Regenerate goods
	auto* pItemList = pVendor->ItemGeneratorAsset->ValidateObject<CItemList>();
	if (!pItemList) return;

	const auto* pVendorEntity = pWorld->GetEntity(VendorID);
	const auto LevelID = pVendorEntity ? pVendorEntity->LevelID : CStrID{};

	auto* pContainer = pWorld->FindComponent<CItemContainerComponent>(pVendor->ContainerID ? pVendor->ContainerID : VendorID);
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

void GetItemPrices(Game::CGameSession& Session, Game::HEntity VendorID, Game::HEntity BuyerID, const std::set<Game::HEntity>& ItemIDs,
	std::map<Game::HEntity, CVendorCoeffs>& Out)
{
	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto* pVendor = pWorld->FindComponent<CVendorComponent>(VendorID);
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
		if (auto Fn = ScriptObject.get<sol::function>("GetItemPrices"))
			Scripting::LuaCall(Fn, VendorID, BuyerID, ItemIDs, BuyFromVendorCoeff, SellToVendorCoeff, Out);

	// Get overrides from vendor factions for items not overridden by the vendor itself
	if (const auto pSocialMgr = Session.FindFeature<CSocialManager>())
		if (const auto* pVendorSocial = pWorld->FindComponent<const CSocialComponent>(VendorID))
			for (const auto FactionID : pVendorSocial->Factions)
				if (const auto* pFaction = pSocialMgr->FindFaction(FactionID))
					if (auto ScriptObject = Session.GetScript(pFaction->ScriptAssetID))
						if (auto Fn = ScriptObject.get<sol::function>("GetItemPrices"))
							Scripting::LuaCall(Fn, VendorID, BuyerID, ItemIDs, BuyFromVendorCoeff, SellToVendorCoeff, Out);

	// Set default currency coefficients
	for (const auto ItemID : ItemIDs)
		if (pWorld->FindComponent<const CCurrencyComponent>(ItemID))
			Out.emplace(ItemID, CVendorCoeffs{ 1.f, 1.f });

	// Set default coeffs if they are not set in scripts
	Out.emplace(Game::HEntity{}, CVendorCoeffs{ BuyFromVendorCoeff, SellToVendorCoeff });
}
//---------------------------------------------------------------------

CItemPrices GetItemStackUnitPrices(Game::CGameSession& Session, Game::HEntity StackID, const CVendorCoeffs& Coeffs)
{
	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
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

U32 CalcStackPrice(U32 UnitPrice, U32 UnitQuantity, U32 Count, bool VendorGoods)
{
	// Fractional vendor goods are always rounded up, party goods are rounded down, to prevent money farming
	if (VendorGoods && UnitQuantity > 1)
		Count += UnitQuantity - 1;

	return UnitPrice * Count / UnitQuantity;
}
//---------------------------------------------------------------------

// Returns remaining cost
I32 GatherMoneyFromPouch(Game::CGameSession& Session, U32 TotalCost, const std::map<Game::HEntity, CVendorCoeffs>& PriceCoeffs,
	const std::map<Game::HEntity, U32>& UsedMoney, std::map<Game::HEntity, U32>& Out)
{
	I32 RemainingPayment = TotalCost;

	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return RemainingPayment;

	auto* pItemMgr = Session.FindFeature<CItemManager>();
	if (!pItemMgr) return RemainingPayment;

	auto* pPouch = pWorld->FindComponent<const CItemContainerComponent>(pItemMgr->GetPartyPouch());
	if (!pPouch) return RemainingPayment;

	// Default currency coeffs are all 1.f
	CVendorCoeffs DefaultCurrencyCoeffs;

	// Sort player money by the unit price
	struct CMoney
	{
		Game::HEntity StackID;
		U32           Price;
		U32           Quantity;
		U32           Count;
	};
	std::vector<CMoney> PlayerMoneyStacks;
	PlayerMoneyStacks.reserve(pPouch->Items.size());
	for (const auto StackID : pPouch->Items)
	{
		auto* pStack = pWorld->FindComponent<const CItemStackComponent>(StackID);
		if (!pStack) continue;

		auto StackCount = pStack->Count;
		auto ItUsed = UsedMoney.find(StackID);
		if (ItUsed != UsedMoney.cend()) StackCount -= ItUsed->second;
		if (!StackCount) continue;

		auto ItCoeff = PriceCoeffs.find(pStack->Prototype);
		const auto& ItemPriceCoeffs = (ItCoeff != PriceCoeffs.cend()) ? ItCoeff->second : DefaultCurrencyCoeffs;
		const auto Prices = GetItemStackUnitPrices(Session, StackID, ItemPriceCoeffs);
		PlayerMoneyStacks.push_back(CMoney{ StackID, Prices.SellToVendorPrice, Prices.SellToVendorQuantity, StackCount });
	}

	std::sort(PlayerMoneyStacks.begin(), PlayerMoneyStacks.end(), [](const auto& a, const auto& b)
	{
		if (a.Price != b.Price) return a.Price < b.Price;
		return a.Quantity > b.Quantity;
	});

	// Calculate total sum of money from lowest ones to the first money with unit price higher than the payment.
	U32 LowerDenominationsCost = 0;
	auto It = PlayerMoneyStacks.begin();
	for (; It != PlayerMoneyStacks.end(); ++It)
	{
		if (static_cast<I32>(It->Price) > RemainingPayment) break;
		LowerDenominationsCost += CalcStackPrice(It->Price, It->Quantity, It->Count, false);
	}

	while (RemainingPayment > 0)
	{
		U32 Count = 0;
		if (static_cast<I32>(LowerDenominationsCost) < RemainingPayment)
		{
			// There is no enough money to pay the remaining sum in lower denominations, use a higher one or fail with not enough money
			if (It == PlayerMoneyStacks.end() || It->Count < It->Quantity) break;
			Count = It->Quantity;
		}
		else
		{
			// There is enough small money. Spend from higher denominations down.
			--It;

			const auto CurrDenominationCost = CalcStackPrice(It->Price, It->Quantity, It->Count, false);
			LowerDenominationsCost -= CurrDenominationCost;

			Count = std::min<I32>(CurrDenominationCost, RemainingPayment) * It->Quantity / It->Price;
			n_assert_dbg(It->Count >= Count);
		}

		It->Count -= Count;

		Out.emplace(It->StackID, Count);
		RemainingPayment -= CalcStackPrice(It->Price, It->Quantity, Count, false);
	}

	return RemainingPayment;
}
//---------------------------------------------------------------------

}
