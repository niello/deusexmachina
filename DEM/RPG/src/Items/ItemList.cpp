#include "ItemList.h"
#include <Items/ItemManager.h>
#include <Items/ItemComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

// Borrows a value from Src to Dest so that the Dest becomes a min of original values of Dest & Src
template<typename T>
static inline void BorrowMinOptional(std::optional<T>& Dest, std::optional<T>& Src)
{
	if (!Src) return;
	Dest = Dest ? std::min(*Src, *Dest) : *Src;
	Src = *Src - *Dest;
}
//---------------------------------------------------------------------

static inline bool IsLimitReached(const CItemLimits& Limits)
{
	return (Limits.Count && !*Limits.Count) ||
		(Limits.Cost && !*Limits.Cost) ||
		(Limits.Weight && *Limits.Weight < 0.0001f) ||
		(Limits.Volume && *Limits.Volume < 0.0001f);
}
//---------------------------------------------------------------------

bool CItemList::EvaluateRecord(const CItemListRecord& Record, Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, U32 Mul, CItemLimits& Limits)
{
	// TODO: can also use XdY+Z to implement non-linear distribution, but for a simple [min; max] it is harder to setup
	U32 Count = ((Record.MinCount < Record.MaxCount) ? Math::RandomU32(Record.MinCount, Record.MaxCount) : Record.MaxCount) * Mul;

	// Intentionally generating 0 items is OK, we should not discard the record
	if (!Count) return true;

	if (Record.ItemTemplateID)
	{
		// Find or allocate a generated item stack record
		auto It = Out.find(Record.ItemTemplateID);
		if (It == Out.end())
		{
			const auto* pItemMgr = Session.FindFeature<CItemManager>();
			if (!pItemMgr) return false;

			const auto ProtoID = pItemMgr->FindPrototypeEntity(Record.ItemTemplateID);
			if (!ProtoID) return false;

			const auto* pWorld = Session.FindFeature<DEM::Game::CGameWorld>();
			if (!pWorld) return false;

			const auto* pItem = pWorld->FindComponent<const CItemComponent>(ProtoID);
			if (!pItem) return false;

			It = Out.emplace(Record.ItemTemplateID, CItemStackData{ ProtoID, pItem, 0 }).first;
		}

		// Clamp the generated count to limits
		if (Limits.Count)
			Count = std::min(Count, *Limits.Count);
		if (Limits.Cost && It->second.pItem->Price)
			Count = std::min(Count, *Limits.Cost / It->second.pItem->Price);
		if (Limits.Weight && It->second.pItem->Weight > 0.f)
			Count = std::min(Count, static_cast<U32>(*Limits.Weight / It->second.pItem->Weight));
		if (Limits.Volume && It->second.pItem->Volume > 0.f)
			Count = std::min(Count, static_cast<U32>(*Limits.Volume / It->second.pItem->Volume));

		// Discard a record if it isn't able to generate more items within limits.
		// NB: the record with 0 count remains in Out and will be cleared at the end. Clearing is
		//     delayed intentionally because this ItemTemplateID may be added by other records.
		if (!Count) return false;

		It->second.Count += Count;

		// Subtract generated items from limits
		if (Limits.Count) Limits.Count = *Limits.Count - Count;
		if (Limits.Cost) Limits.Cost = *Limits.Cost - Count * It->second.pItem->Price;
		if (Limits.Weight) Limits.Weight = *Limits.Weight - Count * It->second.pItem->Weight;
		if (Limits.Volume) Limits.Volume = *Limits.Volume - Count * It->second.pItem->Volume;
	}
	else if (Record.SubList)
	{
		if (auto* pSubList = Record.SubList->ValidateObject<CItemList>())
		{
			const auto Evaluations = Record.SingleEvaluation ? 1 : Count;
			const auto Multiplier = Record.SingleEvaluation ? Count : 1;
			for (U32 i = 0; i < Evaluations; ++i)
			{
				const auto PreBorrowLimits = Limits;
				auto SubLimits = pSubList->ItemLimit;

				// Apply parent limits to the sub-list
				BorrowMinOptional(SubLimits.Count, Limits.Count);
				BorrowMinOptional(SubLimits.Cost, Limits.Cost);
				BorrowMinOptional(SubLimits.Weight, Limits.Weight);
				BorrowMinOptional(SubLimits.Volume, Limits.Volume);

				pSubList->EvaluateInternal(Session, Out, Multiplier, SubLimits);

				// Return remaining part of borrowed limits to the parent
				if (PreBorrowLimits.Count && SubLimits.Count) Limits.Count = *Limits.Count + *SubLimits.Count;
				if (PreBorrowLimits.Cost && SubLimits.Cost) Limits.Cost = *Limits.Cost + *SubLimits.Cost;
				if (PreBorrowLimits.Weight && SubLimits.Weight) Limits.Weight = *Limits.Weight + *SubLimits.Weight;
				if (PreBorrowLimits.Volume && SubLimits.Volume) Limits.Volume = *Limits.Volume + *SubLimits.Volume;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

void CItemList::EvaluateInternal(Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, U32 Mul, CItemLimits& Limits) const
{
	// Record limit is special, it is always local for the current list and neither affects nor is affected by sub-list limits
	U32 RemainingRecords = std::max<U32>(1, RecordLimit.value_or(std::numeric_limits<U32>::max()));

	if (Random)
	{
		// Collect records that will participate in randomized selection
		std::vector<const CItemListRecord*> ValidRecords;
		ValidRecords.reserve(Records.size());
		U32 TotalWeight = 0;
		size_t NonEmptyRecords = 0;
		for (const auto& Record : Records)
		{
			// Records with zero weight should never be chosen. This can be used for disabling records temporarily in a config.
			if (!Record.RandomWeight) continue;

			// When we are not limited by the record count, there is no reason to process empty records, it would only slow things down
			if (!RecordLimit && !Record.MaxCount) continue;

			// TODO: can build a var storage for conditions, maybe outside, e.g. to pass a destination container ID
			if (!Flow::EvaluateCondition(Record.Condition, Session, nullptr)) continue;

			ValidRecords.push_back(&Record);
			TotalWeight += Record.RandomWeight;
			NonEmptyRecords += static_cast<size_t>(Record.MaxCount > 0);
		}

		// Select records randomly until the limit is reached or all records are unable to generate items
		while (NonEmptyRecords)
		{
			const auto Rnd = Math::RandomU32(0, TotalWeight - 1);
			U32 WeightCounter = 0;
			for (auto It = ValidRecords.begin(); It != ValidRecords.end(); ++It)
			{
				const auto& Record = **It;
				WeightCounter += Record.RandomWeight;
				if (WeightCounter > Rnd)
				{
					if (!EvaluateRecord(Record, Session, Out, Mul, Limits))
					{
						// The record is not able to generate more items within remaining limits
						TotalWeight -= Record.RandomWeight;
						NonEmptyRecords -= static_cast<size_t>(Record.MaxCount > 0);
						ValidRecords.erase(It);
					}

					break;
				}
			}

			if (RecordLimit && --RemainingRecords == 0) break;
			if (IsLimitReached(Limits)) break;
		}
	}
	else
	{
		// Scan the whole list once, break if the limit is reached
		for (const auto& Record : Records)
		{
			// TODO: can build a var storage for conditions, maybe outside, e.g. to pass a destination container ID
			if (!Flow::EvaluateCondition(Record.Condition, Session, nullptr)) continue;

			EvaluateRecord(Record, Session, Out, Mul, Limits);

			if (RecordLimit && --RemainingRecords == 0) break;
			if (IsLimitReached(Limits)) break;
		}
	}
}
//---------------------------------------------------------------------

void CItemList::Evaluate(Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, const CItemLimits& Limits, U32 Mul) const
{
	auto LimitsCopy = Limits;
	EvaluateInternal(Session, Out, Mul, LimitsCopy);

	// Clear stacks that don't have any items
	for (auto It = Out.begin(); It != Out.end(); /**/)
	{
		if (It->second.Count)
			++It;
		else
			It = Out.erase(It);
	}
}
//---------------------------------------------------------------------

void CItemList::OnPostLoad(Resources::CResourceManager& ResMgr)
{
	// Random list must be limited. Default case is a single record.
	if (Random && !RecordLimit && !ItemLimit.Count && !ItemLimit.Cost && !ItemLimit.Weight && !ItemLimit.Volume)
		RecordLimit = 1;

	for (auto& Record : Records)
	{
		if (Record.MinCount > Record.MaxCount)
			std::swap(Record.MinCount, Record.MaxCount);

		// Recursively load sub-lists
		ResMgr.RegisterResource<CItemList>(Record.SubList);
		if (Record.SubList)
		{
			Record.SubList->ValidateObject<CItemList>();

			// Can't generate both item and sub-list from the single record
			n_assert(!Record.ItemTemplateID);
			Record.ItemTemplateID = {};
		}
		else if (!Record.ItemTemplateID)
		{
			// Simplify checks by ensuring that empty records always have zero MaxCount
			Record.MinCount = 0;
			Record.MaxCount = 0;
		}
	}
}
//---------------------------------------------------------------------

}
