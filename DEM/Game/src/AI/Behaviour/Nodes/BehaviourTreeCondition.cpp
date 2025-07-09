#include "BehaviourTreeCondition.h"
#include <AI/Behaviour/BehaviourTreePlayer.h>
#include <AI/AIStateComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scripting/Flow/ConditionRegistry.h>
#include <Data/SerializeToParams.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeCondition, 'BTCO', CBehaviourTreeNodeBase);

static const CStrID sidLeft("Left");
static const CStrID sidRight("Right");

// FIXME: move to condition system! Need session for ICondition resolving?!
//        Pass session or Flow::CConditionRegistry into loader on its registration? And move it to CAppStateLoading::Load()!
static void EnumerateUsedContextKeys(const Flow::CConditionData& Condition, std::vector<CStrID>& Out)
{
	// TODO: scripted conditions must report their keys. NEED session for Lua! and ICondition::EnumerateUsedContextKeys.
	if (Condition.Type == Flow::CVarCmpVarCondition::Type)
	{
		Out.push_back(Condition.Params->Get<CStrID>(sidLeft));
		Out.push_back(Condition.Params->Get<CStrID>(sidRight));
	}
	else if (Condition.Type == Flow::CVarCmpConstCondition::Type)
	{
		Out.push_back(Condition.Params->Get<CStrID>(sidLeft));
	}
	else if (Condition.Type == Flow::CAndCondition::Type || Condition.Type == Flow::COrCondition::Type)
	{
		for (const auto& Param : *Condition.Params)
		{
			Flow::CConditionData Inner;
			ParamsFormat::Deserialize(Param.GetRawValue(), Inner);
			EnumerateUsedContextKeys(Inner, Out);
		}
	}
	else if (Condition.Type == Flow::CNotCondition::Type)
	{
		Flow::CConditionData Inner;
		ParamsFormat::Deserialize(Condition.Params, Inner);
		EnumerateUsedContextKeys(Inner, Out);
	}
}
//---------------------------------------------------------------------

void CBehaviourTreeCondition::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	if (auto* pDesc = pParams->Find(CStrID("Condition")))
	{
		ParamsFormat::Deserialize(pDesc->GetRawValue(), _Condition);

		// Gather context (blackboard) keys used by the conditon
		_UsedBBKeys.reserve(16);
		EnumerateUsedContextKeys(_Condition, _UsedBBKeys);
		std::sort(_UsedBBKeys.begin(), _UsedBBKeys.end());
		_UsedBBKeys.erase(std::unique(_UsedBBKeys.begin(), _UsedBBKeys.end()), _UsedBBKeys.end());
		_UsedBBKeys.shrink_to_fit();
	}
}
//---------------------------------------------------------------------

void CBehaviourTreeCondition::OnTreeStarted(U16 SelfIdx, CBehaviourTreePlayer& Player) const
{
	const auto* pConditions = Player.GetSession()->FindFeature<Flow::CConditionRegistry>();
	if (!pConditions) return;

	auto* pCondition = pConditions->FindCondition(_Condition.Type);
	if (!pCondition) return;

	auto* pWorld = Player.GetSession()->FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto* pBrain = pWorld->FindComponent<const CAIStateComponent>(Player.GetActorID());
	if (!pBrain) return;

	//!!!FIXME: here and in quests must ensure that composite conditions subscribe correctly! now it seems that they don't!
	pCondition->SubscribeRelevantEvents(Player.Subscriptions(), { _Condition, *Player.GetSession(), &pBrain->Blackboard.GetStorage() }, [&Player, SelfIdx](const std::shared_ptr<Game::CGameVarStorage>& EventVars)
	{
		// Execution may not even reach this node so we don't check a condition value here
		Player.RequestEvaluation(SelfIdx);
	});

	for (const auto Key : _UsedBBKeys)
		Player.EvaluateOnBlackboardChange(pBrain->Blackboard, Key, SelfIdx);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeCondition::TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext& Ctx) const
{
	// TODO: if condition is async, return SelfIdx to request activation. Maybe a separate type is needed for async conditions not to clutter logic.
	//???Flow::EvaluateConditionAsync? When not supported, will return result immediately. Or not needed and for async can use action decorators with request success check?

	if (Flow::EvaluateCondition(_Condition, Ctx.Session, &Ctx.pBrain->Blackboard.GetStorage()))
		return { EBTStatus::Succeeded, SelfIdx + 1 }; // NB: report immediate success if there is no child
	else
		return { EBTStatus::Failed, SkipIdx };
}
//---------------------------------------------------------------------

}
