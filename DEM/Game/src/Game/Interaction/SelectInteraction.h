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
		if (!Enqueue) Context.SelectedActors.clear();
		if (!Context.IsSelectedActor(Context.SelectedTargets[0].Entity))
			Context.SelectedActors.push_back(Context.SelectedTargets[0].Entity);
		return true;
	}
};

}
