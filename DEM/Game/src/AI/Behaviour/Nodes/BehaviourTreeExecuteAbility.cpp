#include "BehaviourTreeExecuteAbility.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/InteractionManager.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Objects/SmartObject.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <AI/AIStateComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeExecuteAbility, 'BTEA', CBehaviourTreeNodeBase);

void CBehaviourTreeExecuteAbility::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_AbilityID, *pParams, CStrID("Ability"));
	ParameterFromData(_Target, *pParams, CStrID("Target"));
}
//---------------------------------------------------------------------

size_t CBehaviourTreeExecuteAbility::GetInstanceDataSize() const
{
	return sizeof(CCommandFuture);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeExecuteAbility::GetInstanceDataAlignment() const
{
	return alignof(CCommandFuture);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeExecuteAbility::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Cmd = *new(pData) CCommandFuture();

	auto& BB = Ctx.pBrain->Blackboard;
	const auto AbilityID = _AbilityID.Get(BB);
	if (!AbilityID) return EBTStatus::Succeeded;

	auto* pIactMgr = Ctx.Session.FindFeature<Game::CInteractionManager>();
	if (!pIactMgr) return EBTStatus::Failed;

	// Get target
	//???TODO: deserialize CTargetInfo structure right from parameter? or even vector of them!
	Game::CTargetInfo Target;
	if (!_Target.TryGet(BB, Target.Point))
		if (!_Target.TryGet(BB, Target.Entity))
			return EBTStatus::Failed;

	// If the target is an entity, try to get it's smart object ID for correct ability overriding
	CStrID SmartObjectID;
	if (Target.Entity)
	{
		auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
		if (!pWorld) return EBTStatus::Failed;

		if (auto* pSOComponent = pWorld->FindComponent<const Game::CSmartObjectComponent>(Target.Entity))
			if (auto* pSO = pSOComponent->Asset->GetObject<Game::CSmartObject>())
				SmartObjectID = pSO->GetID();
	}

	auto* pAbility = pIactMgr->FindInteraction(AbilityID, SmartObjectID);
	if (!pAbility) return EBTStatus::Failed;

	Game::CInteractionContext IactCtx;
	IactCtx.Actors.push_back(Ctx.ActorID);
	IactCtx.Targets.push_back(std::move(Target));

	if (!pAbility->Execute(Ctx.Session, IactCtx)) return EBTStatus::Failed;

	if (!IactCtx.Commands.empty() && IactCtx.Commands[0])
	{
		Cmd = std::move(IactCtx.Commands[0]);
		return CommandStatusToBTStatus(Cmd.GetStatus());
	}

	// No command was produced but an ability reported success
	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

void CBehaviourTreeExecuteAbility::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Cmd = *reinterpret_cast<CCommandFuture*>(pData);
	Cmd.RequestCancellation();
	std::destroy_at(&Cmd);
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeExecuteAbility::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Cmd = *reinterpret_cast<CCommandFuture*>(pData);
	return { CommandStatusToBTStatus(Cmd.GetStatus()), SelfIdx };
}
//---------------------------------------------------------------------

}
