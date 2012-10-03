#include "GoalIdle.h"

#include <Data/StringID.h>

namespace AI
{
ImplementRTTI(AI::CGoalIdle, AI::CGoal);
ImplementFactory(AI::CGoalIdle);

void CGoalIdle::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Idle"));
}
//---------------------------------------------------------------------

} //namespace AI