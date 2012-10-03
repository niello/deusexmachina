#include "GoalWander.h"

#include <Data/StringID.h>

namespace AI
{
ImplementRTTI(AI::CGoalWander, AI::CGoal);
ImplementFactory(AI::CGoalWander);

void CGoalWander::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

} //namespace AI