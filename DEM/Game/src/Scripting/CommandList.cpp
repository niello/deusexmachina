#include "CommandList.h"
#include <Math/Math.h>

namespace DEM::Game
{

//!!!TODO: to utility/math header!
//!!!need session RNG!
//!!!use uniform distribution!
static inline bool Chance(float Probability)
{
	return (Probability >= 1.f) || (Probability > 0.f && Math::RandomFloat() < Probability);
}
//---------------------------------------------------------------------

bool ExecuteCommandList(const CCommandList& List, CGameSession& Session, CGameVarStorage* pVars /*chance RNG*/)
{
	for (const auto& Record : List)
	{
		if (Chance(Record.Chance /*RNG*/))
			if (EvaluateCondition(Record.Condition, Session, pVars))
				if (ExecuteCommand(Record.Command, Session, pVars))
					continue;

		if (Record.BreakIfFailed) return false;
	}

	return true;
}
//---------------------------------------------------------------------

}
