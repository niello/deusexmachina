#include "BehaviourTreeSelectRandomPosition.h"
#include <AI/Navigation/NavAgentComponent.h>
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

	pParams->TryGet(_DestBBKey, CStrID("DestBBKey"));

	if (auto* pRadiusParam = pParams->Find(CStrID("Radius")))
		_Radius = { pRadiusParam->GetRawValue() };
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

	// TODO: random point can be not only around actor but also around a static point or another actor/object or at the named zone (poly or set of polys)
	// Can determine explicitly or implicitly from params in Init() and remember. Or if it is in BB, check here in Activate, because BB value can change between activations?
	//???for point in nav. region make separate action? radius not needed and CStrID ambiguity between BB key and zone ID will be removed from here in implicit case.
	auto* pActorScene = pWorld->FindComponent<const Game::CSceneComponent>(Ctx.ActorID);
	if (!pActorScene) return EBTStatus::Failed;
	const rtm::vector4f ActorPos = pActorScene->RootNode->GetWorldPosition();
	const auto ActorPosRaw = Math::FromSIMD3(ActorPos);

	// FIXME: can't use provided RNG! Detour accepts only a free function without args.
	// FIXME: not exactly in radius, may be far away if the chosen poly is big. Need more sophisticated logic if this is not acceptable.
	dtPolyRef PolyRef = 0;
	vector3 Point;
	const auto NavStatus = pAgent->pNavQuery->findRandomPointAroundCircle(
		pAgent->Corridor.getFirstPoly(), ActorPosRaw.v, Radius, pAgent->Settings->GetQueryFilter(), Math::RandomFloat, &PolyRef, Point.v);
	if (!dtStatusSucceed(NavStatus)) return EBTStatus::Failed;

	BB.Set(_DestBBKey, Math::ToSIMD(Point));

	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

}
