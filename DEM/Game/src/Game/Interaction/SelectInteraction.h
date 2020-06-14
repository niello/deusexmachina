#pragma once
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CSelectInteraction : public CInteraction
{
public:

	virtual bool Execute(CInteractionContext& Context, bool Enqueue) const override
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
};

}
