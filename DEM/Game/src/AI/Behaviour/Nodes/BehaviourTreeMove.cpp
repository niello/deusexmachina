#include "BehaviourTreeMove.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>
#include <AI/AIStateComponent.h>
#include <Scene/SceneComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeMove, 'BTMV', CBehaviourTreeAIActionBase);

void CBehaviourTreeMove::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_Target, *pParams, CStrID("Target"));
	pParams->TryGet(_IgnoreNavigation, CStrID("IgnoreNavigation"));
	pParams->TryGet(_Follow, CStrID("Follow"));
}
//---------------------------------------------------------------------

size_t CBehaviourTreeMove::GetInstanceDataSize() const
{
	return sizeof(Game::HAction);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeMove::GetInstanceDataAlignment() const
{
	return alignof(Game::HAction);
}
//---------------------------------------------------------------------

//???TODO: use parameter's Visit to avoid searching BB key for each type? Can't cache key, it encodes type!
std::optional<rtm::vector4f> CBehaviourTreeMove::GetPosition(const CBehaviourTreeContext& Ctx) const
{
	const auto& BB = Ctx.pBrain->Blackboard;

	// Directly set target position vector
	{
		rtm::vector4f TargetPos;
		if (_Target.TryGet(BB, TargetPos)) return TargetPos;
	}

	// Position of the target entity
	{
		auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
		if (!pWorld) return {};

		Game::HEntity TargetID;
		if (_Target.TryGet(BB, TargetID))
		{
			if (auto* pTargetScene = pWorld->FindComponent<const Game::CSceneComponent>(TargetID))
				return pTargetScene->RootNode->GetWorldPosition();
			return {};
		}
	}

	return {};
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeMove::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Action = *new(pData) Game::HAction();

	if (!Ctx.pActuator) return EBTStatus::Failed;

	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return EBTStatus::Failed;

	auto OptPos = GetPosition(Ctx);
	if (!OptPos) return EBTStatus::Failed;

	Ctx.pActuator->Reset();

	if (_IgnoreNavigation || !pWorld->FindComponent<const AI::CNavAgentComponent>(Ctx.ActorID))
	{
		Ctx.pActuator->EnqueueAction<AI::Steer>(*OptPos, *OptPos, 0.f);

		//!!!FIXME: we don't even know if it is our action!
		Action = Ctx.pActuator->FindCurrent<AI::Steer>();
	}
	else
	{
		Ctx.pActuator->EnqueueAction<AI::Navigate>(*OptPos, 0.f);

		//!!!FIXME: we don't even know if it is our action!
		Action = Ctx.pActuator->FindCurrent<AI::Navigate>();
	}

	return ActionStatusToBTStatus(Ctx.pActuator->GetStatus(Action));
}
//---------------------------------------------------------------------

void CBehaviourTreeMove::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	CancelAction(Ctx, Action);
	std::destroy_at(&Action);
}
//---------------------------------------------------------------------

// TODO: if not tracking the target, can request next update 'never' and subscribe on action's finish event for re-evaluation/update request
std::pair<EBTStatus, U16> CBehaviourTreeMove::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);

	//???only if not completed yet? what will happen with completed one?
	if (_Follow)
	{
		if (!Ctx.pActuator) return { EBTStatus::Failed, SelfIdx };

		auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
		if (!pWorld) return { EBTStatus::Failed, SelfIdx };

		auto OptPos = GetPosition(Ctx);
		if (!OptPos) return { EBTStatus::Failed, SelfIdx };

		// FIXME: most probably will not work! Need update for the root action too!
		if (_IgnoreNavigation || !pWorld->FindComponent<const AI::CNavAgentComponent>(Ctx.ActorID))
			Ctx.pActuator->PushOrUpdateChild<AI::Steer>({}, *OptPos, *OptPos, 0.f);
		else
			Ctx.pActuator->PushOrUpdateChild<AI::Navigate>({}, *OptPos, 0.f);
	}

	return { GetActionStatus(Ctx, Action), SelfIdx };
}
//---------------------------------------------------------------------

}
