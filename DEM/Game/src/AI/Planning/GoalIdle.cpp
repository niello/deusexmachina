#include "GoalIdle.h"

#include <Data/StringID.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CGoalIdle, 'GIDL', AI::CGoal);

void CGoalIdle::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Idle"));
}
//---------------------------------------------------------------------

}