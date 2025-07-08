#include "BehaviourTreeSelectRandomPosition.h"
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/NavMesh.h>
#include <AI/AIStateComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeSelectRandomPosition, 'BTRP', CBehaviourTreeNodeBase);

void CBehaviourTreeSelectRandomPosition::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_From, *pParams, CStrID("From"));
	ParameterFromData(_Radius, *pParams, CStrID("Radius"));
	pParams->TryGet(_DestBBKey, CStrID("DestBBKey"));
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeSelectRandomPosition::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	if (!_DestBBKey) return EBTStatus::Failed;

	auto& BB = Ctx.pBrain->Blackboard;
	const float Radius = _Radius.Get(BB);

	if (!_DestBBKey || Radius <= 0.f) return EBTStatus::Failed;

	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return EBTStatus::Failed;

	auto* pAgent = pWorld->FindComponent<const AI::CNavAgentComponent>(Ctx.ActorID);
	if (!pAgent) return EBTStatus::Failed;

	vector3 ResultPoint;

	//???!!!FIXME: for point in nav. region make separate action? radius not needed, logic is different.
	CStrID NavRegionID;
	if (_From.TryGet(BB, NavRegionID))
	{
		const auto* pRegion = pAgent->NavMap->GetNavMesh()->FindRegion(NavRegionID);
		if (!pRegion) return EBTStatus::Failed;

		// pRegion is a set of poly refs, need the same logic as in findRandomPointAroundCircle but over the predefined set of polys

		NOT_IMPLEMENTED;
		return EBTStatus::Failed;
	}
	else
	{
		// Default behaviour is to search around an actor, but another entity can be chosen explicitly
		Game::HEntity EntityID;
		if (_From.IsEmpty())
			EntityID = Ctx.ActorID;
		else
			_From.TryGet(BB, EntityID);

		rtm::vector4f CenterPos;
		if (EntityID)
		{
			auto* pSceneComponent = pWorld->FindComponent<const Game::CSceneComponent>(EntityID);
			if (!pSceneComponent) return EBTStatus::Failed;
			CenterPos = pSceneComponent->RootNode->GetWorldPosition();
		}
		else
		{
			// The last our option is an explicit position
			if (!_From.TryGet(BB, CenterPos)) return EBTStatus::Failed;
		}

		// FIXME: can't use provided RNG! Detour accepts only a free function without args.
		// FIXME: not exactly in radius, may be far away if the chosen poly is big. Need more sophisticated logic if this is not acceptable.
		dtPolyRef PolyRef = 0;
		const auto NavStatus = pAgent->pNavQuery->findRandomPointAroundCircle(
			pAgent->Corridor.getFirstPoly(), Math::FromSIMD3(CenterPos).v, Radius, pAgent->Settings->GetQueryFilter(), Math::RandomFloat, &PolyRef, ResultPoint.v);
		if (!dtStatusSucceed(NavStatus)) return EBTStatus::Failed;
	}

	BB.Set(_DestBBKey, ResultPoint);

	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

}
