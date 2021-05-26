#include "SelectInteraction.h"
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/SelectableComponent.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::Game
{

CSelectInteraction::CSelectInteraction(std::string_view CursorImage)
{
	_Name = "Select"; //???user-settable?
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

static bool IsTargetSelectable(const CGameSession& Session, const CInteractionContext& Context, U32 Index)
{
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<CGameWorld>();
	return pWorld && pWorld->FindComponent<CSelectableComponent>(Target.Entity);
}
//---------------------------------------------------------------------

bool CSelectInteraction::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return Index == 0 && IsTargetSelectable(Session, Context, Index);
}
//---------------------------------------------------------------------

ESoftBool CSelectInteraction::NeedMoreTargets(const CInteractionContext& Context) const
{
	return (Context.Targets.size() < 1) ? ESoftBool::True : ESoftBool::False;
}
//---------------------------------------------------------------------

bool CSelectInteraction::Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty()) return false;

	if (!Enqueue) Context.Actors.clear();

	for (const auto& Target : Context.Targets)
		if (Target.Entity && std::find(Context.Actors.cbegin(), Context.Actors.cend(), Target.Entity) == Context.Actors.cend())
			Context.Actors.push_back(Target.Entity);

	return true;
}
//---------------------------------------------------------------------

}
