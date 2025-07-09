#include "BehaviourTreeExecuteAbility.h"
#include <Game/GameSession.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/InteractionManager.h>
#include <Game/Interaction/InteractionContext.h>
//#include <Data/SerializeToParams.h> - deserialize CTargetInfo structure?
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeExecuteAbility, 'BTEA', CBehaviourTreeAIActionBase);

void CBehaviourTreeExecuteAbility::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	pParams->TryGet(_AbilityID, CStrID("Ability"));

	// Target hardcoded ID / hardcoded list of IDs / BB key
}
//---------------------------------------------------------------------

size_t CBehaviourTreeExecuteAbility::GetInstanceDataSize() const
{
	return sizeof(Game::HAction);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeExecuteAbility::GetInstanceDataAlignment() const
{
	return alignof(Game::HAction);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeExecuteAbility::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Action = *new(pData) Game::HAction();

	if (!_AbilityID) return EBTStatus::Succeeded;

	Game::CInteractionManager* pIactMgr = Ctx.Session.FindFeature<Game::CInteractionManager>();
	if (!pIactMgr) return EBTStatus::Failed;

	// resolve TargetEntityID(s) from hardcode or from BB key

	//!!!TODO!
	CStrID SmartObjectID;
	// if (auto pSO = pWorld->FindComponent<const Game::CSmartObjectComponent>(TargetEntityID))
	//	SmartObjectID = pSO->Asset->GetObject<Game::CSmartObject>()->GetID();

	auto pAbility = pIactMgr->FindInteraction(_AbilityID, SmartObjectID);
	if (!pAbility) return EBTStatus::Failed;

	Game::CInteractionContext IactCtx;
	IactCtx.Actors.push_back(Ctx.ActorID);
	//IactCtx.Targets.push_back(TargetEntityID);

	//!!!FIXME: must return HAction? Only abilities? Interactions must not! Or need other way to get HAction from launched ability.
	if (!pAbility->Execute(Ctx.Session, IactCtx, false)) return EBTStatus::Failed;

	//!!!also has pAIState->_AbilityInstance but there is no status there! At least now.
	if (Ctx.pActuator)
	{
		//!!!FIXME: we don't even know if it is our action!
		Action = Ctx.pActuator->FindCurrent<Game::ExecuteAbility>();
		return ActionStatusToBTStatus(Ctx.pActuator->GetStatus(Action));
	}

	// No running action is found
	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

void CBehaviourTreeExecuteAbility::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	CancelAction(Ctx, Action);
	std::destroy_at(&Action);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeExecuteAbility::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	return { GetActionStatus(Ctx, Action), SelfIdx };
}
//---------------------------------------------------------------------

}
