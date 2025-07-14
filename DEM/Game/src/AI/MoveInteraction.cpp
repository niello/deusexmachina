#include "MoveInteraction.h"
#include <Game/Interaction/InteractionContext.h>
#include <Game/ECS/GameWorld.h>
#include <AI/FormationManager.h>
#include <AI/Navigation/NavAgentComponent.h>

namespace DEM::Game
{

CMoveInteraction::CMoveInteraction(std::string_view CursorImage)
{
	_Name = "Move"; //???user-settable?
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

static bool IsTargetPassable(const CGameSession& Session, const CInteractionContext& Context, U32 Index)
{
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;

	const auto TargetPt = Math::FromSIMD3(Target.Point);

	for (HEntity Actor : Context.Actors)
	{
		auto pAgent = pWorld->FindComponent<DEM::AI::CNavAgentComponent>(Actor);
		if (!pAgent || !pAgent->pNavQuery) continue;

		const float Extents[3] = { 0.f, pAgent->Height, 0.f };
		dtPolyRef PolyRef = 0;
		vector3 Pos;
		pAgent->pNavQuery->findNearestPoly(TargetPt.v, Extents, pAgent->Settings->GetQueryFilter(),
			&PolyRef, Pos.v);

		// FIXME: can return point over the impassable object if it stands on the navmesh!
		if (PolyRef &&
			vector3::SqDistance2D(Pos, TargetPt) <= 0.001f * 0.001f &&
			std::abs(Pos.y - TargetPt.y) < pAgent->Height)
		{
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------

bool CMoveInteraction::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return Index == 0 && IsTargetPassable(Session, Context, Index);
}
//---------------------------------------------------------------------

bool CMoveInteraction::Execute(CGameSession& Session, CInteractionContext& Context) const
{
	if (Context.Targets.empty()) return false;

	auto pFormationMgr = Session.FindFeature<CFormationManager>();
	if (!pFormationMgr) return false;

	//!!!Direction (in another CMoveAndFaceInteraction? other input type, drag mouse):
	//const auto* pSceneComponent = pWorld->FindComponent<DEM::Game::CSceneComponent>(*_SelectedActors.begin());
	//const vector3 Direction = Context.SelectedTargets[0].Point - pSceneComponent->RootNode->GetWorldPosition();

	return pFormationMgr->Move(Context.Targets[0].Point, rtm::vector_zero(), Context.Actors, &Context.Commands, Context.Enqueue);
}
//---------------------------------------------------------------------

}
