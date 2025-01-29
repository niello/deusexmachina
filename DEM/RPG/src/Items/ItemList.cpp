#include "ItemList.h"
#include <Items/ItemManager.h>
#include <Game/GameSession.h>

namespace DEM::RPG
{

void CItemList::EvaluateInternal(Game::CGameSession& Session, std::map<CStrID, U32>& Out, U32 Mul, CLimitAccumulator& Limits)
{
	auto pItemMgr = Session.FindFeature<CItemManager>();
	if (!pItemMgr) return;

	if (Random)
	{
		// iterate valid records, pick random ones until the limit is reached

		NOT_IMPLEMENTED;
	}
	else
	{
		for (const auto& Record : Records)
		{
			//!!!handle record limit here! empty records count!

			if (!Record.ItemTemplateID && !Record.SubList) continue;

			// TODO: can also use XdY+Z to implement non-linear distribution, but for a simple [min; max] it is harder to setup
			const auto Count = Math::RandomU32(Record.MinCount, Record.MaxCount) * Mul;
			if (!Count) return;

			// TODO: can build a var storage for conditions, maybe outside, e.g. to pass a destination container ID
			if (!Flow::EvaluateCondition(Record.Condition, Session, nullptr)) continue;

			if (Record.ItemTemplateID)
			{
				Out[Record.ItemTemplateID] += Count;

				if (const auto ProtoID = pItemMgr->FindPrototypeEntity(Record.ItemTemplateID))
				{
					// get item params
				}

				// update limits; may need to cache item's params in an Out map value for faster processing!
				// Limits.x += ...
			}
			else if (Record.SubList)
			{
				if (auto* pSubList = Record.SubList->ValidateObject<CItemList>())
				{
					if (Record.SingleEval)
						pSubList->EvaluateInternal(Session, Out, Count, Limits);
					else
						for (U32 i = 0; i < Count; ++i)
							pSubList->EvaluateInternal(Session, Out, 1, Limits);
				}
			}

			// check limits, if any is recahed, stop
		}
	}
}
//---------------------------------------------------------------------

void CItemList::Evaluate(Game::CGameSession& Session, std::map<CStrID, U32>& Out)
{
	CLimitAccumulator Limits;
	//!!!TODO: init limits!

	//!!!can return Out with entity keys, already obtain item proto IDs!

	EvaluateInternal(Session, Out, 1, Limits);
}
//---------------------------------------------------------------------

}
