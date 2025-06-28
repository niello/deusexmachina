#include "BehaviourTreeExecuteAbility.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/InteractionManager.h>
#include <Game/Interaction/InteractionContext.h>
//#include <Data/SerializeToParams.h> - deserialize target structure?
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeExecuteAbility, 'BTEA', CBehaviourTreeNodeBase);

static EBTStatus ActionStatusToBTStatus(Game::EActionStatus Status)
{
	switch (Status)
	{
		//!!!FIXME: can't know WHY there is no action in queue. Might have succeeded and was removed? Or failed? Or cancelled?
		case Game::EActionStatus::NotQueued:
		case Game::EActionStatus::Succeeded:
			return EBTStatus::Succeeded;
		case Game::EActionStatus::Failed:
		case Game::EActionStatus::Cancelled:
			return EBTStatus::Failed;
		default:
			return EBTStatus::Running;
	}
}
//---------------------------------------------------------------------

void CBehaviourTreeExecuteAbility::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	// Ability ID
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
	auto& Action = *new(pData) Game::HAction();

	Game::CInteractionManager* pIactMgr = Ctx.Session.FindFeature<Game::CInteractionManager>();
	if (!pIactMgr) return EBTStatus::Failed;

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
	if (auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>())
	{
		if (auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID))
		{
			Action = pQueue->FindCurrent<Game::ExecuteAbility>();
			return ActionStatusToBTStatus(pQueue->GetStatus(Action));
		}
	}

	// No running action is found
	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

void CBehaviourTreeExecuteAbility::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);

	if (auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>())
		if (auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID))
			if (pQueue->GetStatus(Action) == Game::EActionStatus::Active)
				pQueue->SetStatus(Action, Game::EActionStatus::Cancelled);

	std::destroy_at(&Action);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeExecuteAbility::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);

	EBTStatus Status = EBTStatus::Running;

	//!!!also has pAIState->_AbilityInstance but there is no status there! At least now.
	if (auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>())
		if (auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID))
			Status = ActionStatusToBTStatus(pQueue->GetStatus(Action));

	return { Status, SelfIdx };
}
//---------------------------------------------------------------------

}
