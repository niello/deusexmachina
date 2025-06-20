#include "BehaviourTreeSequence.h"
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeSequence, 'BTSQ', CBehaviourTreeNodeBase);

U16 CBehaviourTreeSequence::Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, EStatus ChildStatus) const
{
	// Traversing from above means starting from the beginning
	if (PrevIdx < SelfIdx) return SelfIdx + 1;

	if (ChildStatus == EStatus::Succeeded)
	{
		// return skip index of succeeded child, last child's skip index is equal to ours
	}
	else
	{
		// propagate this status up and return own skip index
	}

	return 0;
}
//---------------------------------------------------------------------

}
