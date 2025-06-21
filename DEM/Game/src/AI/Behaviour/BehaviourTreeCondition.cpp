#include "BehaviourTreeCondition.h"
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeCondition, 'BTCO', CBehaviourTreeNodeBase);

U16 CBehaviourTreeCondition::Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus) const
{
	if (PrevIdx < SelfIdx)
	{
		//process condition
		//if condition is async, return SelfIdx to request activation. Maybe a separate type is needed for async conditions not tu clutter logic.
		//if returned false, return SkipIdx with failure
		//else if SelfIdx + 1 == SkipIdx, return SkipIdx with success (no children, should look the same as if we have successfully executed them)
		//else return SelfIdx + 1; (proceed to the first child)
		return SelfIdx + 1;
	}

	// When the child returns, simply propagate its result up
	return NextIdx; // status is passed unchanged
}
//---------------------------------------------------------------------

}
