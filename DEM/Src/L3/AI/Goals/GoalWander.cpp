#include "GoalWander.h"

#include <Data/StringID.h>

namespace AI
{
__ImplementClassNoFactory(AI::CGoalWander, AI::CGoal);
__ImplementClass(AI::CGoalWander);

void CGoalWander::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

} //namespace AI