#include "ItemList.h"
#include <Items/ItemManager.h>
#include <Items/ItemComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

bool CItemList::EvaluateRecord(const CItemListRecord& Record, Game::CGameSession& Session, std::map<CStrID, CItemRecord>& Out, U32 Mul, CItemLimits& Limits)
{
	// TODO: can also use XdY+Z to implement non-linear distribution, but for a simple [min; max] it is harder to setup
	const auto Count = ((Record.MinCount < Record.MaxCount) ? Math::RandomU32(Record.MinCount, Record.MaxCount) : Record.MaxCount) * Mul;
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

			It = Out.emplace(Record.ItemTemplateID, CItemRecord{ ProtoID, pItem, 0 }).first;
		}

		It->second.Count += Count;

		//It->second.pItem->Price;
		//It->second.pItem->Volume;
		//It->second.pItem->Weight;
		// update limits; may need to cache item's params in an Out map value for faster processing!
		// Limits.x += ...
	}
	else if (Record.SubList)
	{
		if (auto* pSubList = Record.SubList->ValidateObject<CItemList>())
		{
			const auto Evaluations = Record.SingleEvaluation ? 1 : Count;
			const auto Multiplier = Record.SingleEvaluation ? Count : 1;
			for (U32 i = 0; i < Evaluations; ++i)
			{
				CItemLimits SubLimits;
				pSubList->EvaluateInternal(Session, Out, Multiplier, SubLimits);

				//!!!add to Limits, check! what if generated more? should have clamped local limits with global?!
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

void CItemList::EvaluateInternal(Game::CGameSession& Session, std::map<CStrID, CItemRecord>& Out, U32 Mul, CItemLimits& Limits)
{
	U32 RemainingRecords = std::max<U32>(1, RecordLimit.value_or(std::numeric_limits<U32>::max()));

	if (Random)
	{
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

			// check limits
		}
	}
	else
	{
		for (const auto& Record : Records)
		{
			// TODO: can build a var storage for conditions, maybe outside, e.g. to pass a destination container ID
			if (!Flow::EvaluateCondition(Record.Condition, Session, nullptr)) continue;

			EvaluateRecord(Record, Session, Out, Mul, Limits);

			if (RecordLimit && --RemainingRecords == 0) break;

			//if (Limits.RecordCount >= RecordLimit || Limits.ItemCount >= ItemLimit || 
			// check limits, if any is recahed, stop
		}
	}
}
//---------------------------------------------------------------------

void CItemList::Evaluate(Game::CGameSession& Session, std::map<CStrID, U32>& Out, U32 Mul)
{
	CItemLimits Limits;

	std::map<CStrID, CItemRecord> OutInternal;
	EvaluateInternal(Session, OutInternal, Mul, Limits);

	//???return OutInternal without conversion?
}
//---------------------------------------------------------------------

void CItemList::OnPostLoad(Resources::CResourceManager& ResMgr)
{
	if (Random)
	{
		//!!!TODO: if random list and has no limits, must limit itself to 1 record.
		//!!!need to define what is "no limits". Zero?
	}

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
			Record.MinCount = 0;
			Record.MaxCount = 0;
		}
	}
}
//---------------------------------------------------------------------

}
