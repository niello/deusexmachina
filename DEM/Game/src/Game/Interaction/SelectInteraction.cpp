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
	if (Context.SelectedTargets.empty()) return false;

	if (auto EntityID = Context.SelectedTargets[0].Entity)
	{
		auto& Selected = Context.SelectedActors;
		if (!Enqueue) Selected.clear();
		if (std::find(Selected.cbegin(), Selected.cend(), EntityID) == Selected.cend())
			Selected.push_back(EntityID);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------

}
