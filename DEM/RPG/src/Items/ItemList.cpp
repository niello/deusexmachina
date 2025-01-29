#include "ItemList.h"

namespace DEM::RPG
{

void CItemList::Evaluate(Game::CGameSession& Session, std::map<CStrID, U32>& Out)
{
	// TODO: can build a var storage for a conditions, maybe outside, e.g. to pass a destination container ID

	if (Random)
	{
		// iterate valid records, pick random ones until the limit is reached
	}
	else
	{
		// iterate valid and non-empty records once from start to end until the limit is reached

		for (const auto& Record : Records)
		{
			if (!Flow::EvaluateCondition(Record.Condition, Session, nullptr)) continue;

			//
		}
	}
}
//---------------------------------------------------------------------

}
