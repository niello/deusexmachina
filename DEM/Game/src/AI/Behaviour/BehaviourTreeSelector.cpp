#include "BehaviourTreeSelector.h"
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeSelector, 'BTSL', CBehaviourTreeNodeBase);

U16 CBehaviourTreeSelector::Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus) const
{
	// Traversing from above always means starting from the beginning
	if (PrevIdx < SelfIdx) return SelfIdx + 1;

	// If the child failed, proceed to the next child or to the subtree end if it was the last child
	//???set status to Running if NextIdx < SkipIdx? or process it outside based on indices?
	if (ChildStatus == EStatus::Failed) return NextIdx;

	// If the child succeeded or is running, skip the whole sequence subtree and return to the parent
	return SkipIdx;
}
//---------------------------------------------------------------------

}
