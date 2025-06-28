#include "BehaviourTreeCondition.h"
#include <AI/Behaviour/BehaviourTreePlayer.h>
#include <Game/GameSession.h>
#include <Scripting/Flow/ConditionRegistry.h>
#include <Data/SerializeToParams.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeCondition, 'BTCO', CBehaviourTreeNodeBase);

void CBehaviourTreeCondition::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	if (auto* pDesc = pParams->Find(CStrID("Condition")))
		DEM::ParamsFormat::Deserialize(pDesc->GetRawValue(), _Condition);
}
//---------------------------------------------------------------------

void CBehaviourTreeCondition::OnTreeStarted(U16 SelfIdx, CBehaviourTreePlayer& Player, const CBehaviourTreeContext& Ctx) const
{
	const auto* pConditions = Ctx.Session.FindFeature<Flow::CConditionRegistry>();
	if (!pConditions) return;

	auto* pCondition = pConditions->FindCondition(_Condition.Type);
	if (!pCondition) return;

	//!!!FIXME: here and in quests must ensure that composite conditions subscribe correctly! now it seems that they don't!
	pCondition->SubscribeRelevantEvents(Player.Subscriptions(), { _Condition, Ctx.Session, nullptr/*Ctx.pBrain->Blackboard*/ }, [&Player, SelfIdx](const std::shared_ptr<CBasicVarStorage>& EventVars)
	{
		// Execution may not even reach this node so we don't check a condition value here
		Player.RequestEvaluation(SelfIdx);
	});
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
