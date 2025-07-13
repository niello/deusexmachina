#include "BehaviourTreeTurn.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <AI/Movement/SteerAction.h>
#include <AI/AIStateComponent.h>
#include <AI/CommandStackComponent.h>
#include <Scene/SceneComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeTurn, 'BTTR', CBehaviourTreeNodeBase);

void CBehaviourTreeTurn::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_Target, *pParams, CStrID("Target"));
	pParams->TryGet(_Follow, CStrID("Follow"));
}
//---------------------------------------------------------------------

size_t CBehaviourTreeTurn::GetInstanceDataSize() const
{
	return sizeof(CCommandFuture);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeTurn::GetInstanceDataAlignment() const
{
	return alignof(CCommandFuture);
}
//---------------------------------------------------------------------

//???TODO: use parameter's Visit to avoid searching BB key for each type? Can't cache key, it encodes type!
std::optional<rtm::vector4f> CBehaviourTreeTurn::GetDirection(const CBehaviourTreeContext& Ctx) const
{
	const auto& BB = Ctx.pBrain->Blackboard;

	// Directly set direction vector
	//???or should it be point to look at?
	{
		rtm::vector4f TargetDir;
		if (_Target.TryGet(BB, TargetDir)) return TargetDir;
	}

	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return {};

	auto* pActorScene = pWorld->FindComponent<const Game::CSceneComponent>(Ctx.ActorID);
	if (!pActorScene) return {};

	// Direction to the target entity
	{
		Game::HEntity TargetID;
		if (_Target.TryGet(BB, TargetID))
		{
			if (auto* pTargetScene = pWorld->FindComponent<const Game::CSceneComponent>(TargetID))
				return rtm::vector_sub(pTargetScene->RootNode->GetWorldPosition(), pActorScene->RootNode->GetWorldPosition());
			return {};
		}
	}

	// An angle from the current direction
	//???or should it be angle from absolute -Z (north)?
	// TODO: use radians and a hint in the editor that the field is an angle?
	{
		float AngleDeg;
		if (_Target.TryGet(BB, AngleDeg))
		{
			const float Angle = n_deg2rad(AngleDeg);
			const auto TargetPos = Math::vector_rotated_xz(rtm::matrix_mul_point3(rtm::vector_set(0.f, 0.f, -1.f), pActorScene->RootNode->GetWorldMatrix()), Angle);
			return rtm::vector_sub(TargetPos, pActorScene->RootNode->GetWorldPosition());
		}
	}

	return {};
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeTurn::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Cmd = *new(pData) CCommandFuture();

	if (!Ctx.pActuator) return EBTStatus::Failed;

	auto OptDir = GetDirection(Ctx);
	if (!OptDir) return EBTStatus::Failed;

	Ctx.pActuator->PopCommand(0, ECommandStatus::Cancelled); //???or shortcut method Reset?
	Cmd = Ctx.pActuator->PushCommand<AI::Turn>(*OptDir, AI::Turn::AngularTolerance);

	return CommandStatusToBTStatus(Cmd.GetStatus());
}
//---------------------------------------------------------------------

void CBehaviourTreeTurn::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	auto& Cmd = *reinterpret_cast<CCommandFuture*>(pData);
	Cmd.RequestCancellation();
	std::destroy_at(&Cmd);
}
//---------------------------------------------------------------------

// TODO: if not tracking the target, can request next update 'never' and subscribe on action's finish event for re-evaluation/update request
std::pair<EBTStatus, U16> CBehaviourTreeTurn::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Cmd = *reinterpret_cast<CCommandFuture*>(pData);

	//???only if not completed yet? what will happen with completed one?
	if (_Follow)
	{
		if (!Ctx.pActuator) return { EBTStatus::Failed, SelfIdx };

		auto OptDir = GetDirection(Ctx);
		if (!OptDir) return { EBTStatus::Failed, SelfIdx };

		Cmd.As<AI::Turn>()->_LookatDirection = *OptDir;
	}

	return { CommandStatusToBTStatus(Cmd.GetStatus()), SelfIdx };
}
//---------------------------------------------------------------------

}
