#include "BehaviourTreeMove.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeMove, 'BTMV', CBehaviourTreeAIActionBase);

void CBehaviourTreeMove::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	pParams->TryGet(_IgnoreNavigation, CStrID("IgnoreNavigation"));

	// Hardcoded position or entity ID or BB key of them
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

EBTStatus CBehaviourTreeMove::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Action = *new(pData) Game::HAction();

	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return EBTStatus::Failed;

	auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID);
	if (!pQueue) return EBTStatus::Failed;

	//!!!TODO: read from BB or hardcoded value!
	rtm::vector4f WorldPosition = rtm::vector_zero();

	pQueue->Reset();

	if (_IgnoreNavigation || !pWorld->FindComponent<const AI::CNavAgentComponent>(Ctx.ActorID))
	{
		pQueue->EnqueueAction<AI::Steer>(WorldPosition, WorldPosition, 0.f);

		//!!!FIXME: we don't even know if it is our action!
		Action = pQueue->FindCurrent<AI::Steer>();
	}
	else
	{
		pQueue->EnqueueAction<AI::Navigate>(WorldPosition, 0.f);

		//!!!FIXME: we don't even know if it is our action!
		Action = pQueue->FindCurrent<AI::Navigate>();
	}

	return ActionStatusToBTStatus(pQueue->GetStatus(Action));
}
//---------------------------------------------------------------------

void CBehaviourTreeMove::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	CancelAction(Ctx, Action);
	std::destroy_at(&Action);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeMove::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	return { GetActionStatus(Ctx, Action), SelfIdx };
}
//---------------------------------------------------------------------

}
