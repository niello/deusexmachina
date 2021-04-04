#include "MoveInteraction.h"
#include <AI/PassableTargetFilter.h>
#include <AI/FormationManager.h>

namespace DEM::Game
{

CMoveInteraction::CMoveInteraction(std::string_view CursorImage)
{
	_Name = "Move"; //???user-settable?
	_CursorImage = CursorImage;
	AddTarget(std::make_unique<CPassableTargetFilter>());
}
//---------------------------------------------------------------------

bool CMoveInteraction::Execute(CInteractionContext& Context, bool Enqueue) const
{
	if (Context.Targets.empty()) return false;

	auto pFormationMgr = Context.Session->FindFeature<CFormationManager>();
	if (!pFormationMgr) return false;

	//!!!Direction (in another CMoveAndFaceInteraction? other input type, drag mouse):
	//const auto* pSceneComponent = pWorld->FindComponent<DEM::Game::CSceneComponent>(*_SelectedActors.begin());
	//const vector3 Direction = Context.SelectedTargets[0].Point - pSceneComponent->RootNode->GetWorldPosition();

	return pFormationMgr->Move(Context.Actors, Context.Targets[0].Point, vector3::Zero, Enqueue);
}
//---------------------------------------------------------------------

}
