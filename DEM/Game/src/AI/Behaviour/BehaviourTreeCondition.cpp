#include "BehaviourTreeCondition.h"
#include <Data/SerializeToParams.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeCondition, 'BTCO', CBehaviourTreeNodeBase);

void CBehaviourTreeCondition::Init(const Data::CParams* pParams)
{
	if (pParams)
		if (auto* pDesc = pParams->Find(CStrID("Condition")))
			DEM::ParamsFormat::Deserialize(pDesc->GetRawValue(), _Condition);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeCondition::TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext& Ctx) const
{
	// TODO: if condition is async, return SelfIdx to request activation. Maybe a separate type is needed for async conditions not to clutter logic.
	//???Flow::EvaluateConditionAsync? When not supported, will return result immediately. Or not needed and for async can use action decorators with request success check?

	if (Flow::EvaluateCondition(_Condition, Ctx.Session, nullptr/*Ctx.pBrain->Blackboard*/))
		return { EBTStatus::Succeeded, SelfIdx + 1 }; // NB: report immediate success if there is no child
	else
		return { EBTStatus::Failed, SkipIdx };
}
//---------------------------------------------------------------------

}
