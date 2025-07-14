#include "BehaviourTreeExecuteAbility.h"
#include <Game/GameSession.h>
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/InteractionManager.h>
#include <Game/Interaction/InteractionContext.h>
#include <AI/AIStateComponent.h>
#include <AI/CommandStackComponent.h>
//#include <Data/SerializeToParams.h> - deserialize CTargetInfo structure?
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeExecuteAbility, 'BTEA', CBehaviourTreeNodeBase);

void CBehaviourTreeExecuteAbility::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_AbilityID, *pParams, CStrID("Ability"));
	//ParameterFromData(_Target, *pParams, CStrID("Target"));
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

	Game::CInteractionManager* pIactMgr = Ctx.Session.FindFeature<Game::CInteractionManager>();
	if (!pIactMgr) return EBTStatus::Failed;

	//!!!TODO: may not be entity. Build CTargetInfo!
	const Game::HEntity TargetEntityID = {};// _Target.Get(BB);

	//!!!TODO!
	CStrID SmartObjectID;
	// if (auto pSO = pWorld->FindComponent<const Game::CSmartObjectComponent>(TargetEntityID))
	//	SmartObjectID = pSO->Asset->GetObject<Game::CSmartObject>()->GetID();

	auto pAbility = pIactMgr->FindInteraction(AbilityID, SmartObjectID);
	if (!pAbility) return EBTStatus::Failed;

	Game::CInteractionContext IactCtx;
	IactCtx.Actors.push_back(Ctx.ActorID);
	IactCtx.Targets.push_back(TargetEntityID);

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
