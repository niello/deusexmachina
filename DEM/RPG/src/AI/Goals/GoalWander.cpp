#include "GoalWander.h"

#include <Data/StringID.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CGoalWander, 'GWDR', AI::CGoal);

void CGoalWander::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

} //namespace AI