#include "BehaviourTreeTurn.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Movement/SteerAction.h>
#include <AI/AIStateComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeTurn, 'BTTR', CBehaviourTreeAIActionBase);

void CBehaviourTreeTurn::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	// Hardcoded position of interest or entity ID or float angle -180..180 or BB key of them
	//_Target = { CStrID("k"), 5 };
}
//---------------------------------------------------------------------

size_t CBehaviourTreeTurn::GetInstanceDataSize() const
{
	return sizeof(Game::HAction);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeTurn::GetInstanceDataAlignment() const
{
	return alignof(Game::HAction);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeTurn::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Action = *new(pData) Game::HAction();

	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return EBTStatus::Failed;

	auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID);
	if (!pQueue) return EBTStatus::Failed;

	//!!!TODO: read from BB or hardcoded value!
	auto& BB = Ctx.pBrain->Blackboard;
	//_TargetA.Get(BB);
	//_TargetB.Get(BB);
	rtm::vector4f TargetDir = rtm::vector_zero();
	const auto FacingTolerance = AI::Turn::AngularTolerance; // std::max(FacingTolerance, Turn::AngularTolerance);

	pQueue->Reset();

	pQueue->EnqueueAction<AI::Turn>(TargetDir, FacingTolerance);

	//!!!FIXME: we don't even know if it is our action!
	Action = pQueue->FindCurrent<AI::Turn>();

	return ActionStatusToBTStatus(pQueue->GetStatus(Action));
}
//---------------------------------------------------------------------

void CBehaviourTreeTurn::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	CancelAction(Ctx, Action);
	std::destroy_at(&Action);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeTurn::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Action = *reinterpret_cast<Game::HAction*>(pData);
	return { GetActionStatus(Ctx, Action), SelfIdx };
}
//---------------------------------------------------------------------

}
