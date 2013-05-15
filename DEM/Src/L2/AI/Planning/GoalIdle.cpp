#include "GoalIdle.h"

#include <Data/StringID.h>

namespace AI
{
__ImplementClassNoFactory(AI::CGoalIdle, AI::CGoal);
__ImplementClass(AI::CGoalIdle);

void CGoalIdle::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Idle"));
}
//---------------------------------------------------------------------

} //namespace AI