#include "SelectInteraction.h"
#include <Game/Interaction/SelectableTargetFilter.h>

namespace DEM::Game
{

CSelectInteraction::CSelectInteraction(std::string_view CursorImage)
{
	_Name = "Select"; //???user-settable?
	_CursorImage = CursorImage;
	AddTarget(std::make_unique<CSelectableTargetFilter>());
}
//---------------------------------------------------------------------

bool CSelectInteraction::Execute(CInteractionContext& Context, bool Enqueue) const
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
