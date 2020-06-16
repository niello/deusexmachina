#pragma once
#include <Game/Interaction/Interaction.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CSelectInteraction : public CInteraction
{
public:

	CSelectInteraction(std::string_view CursorImage = {});

	virtual bool Execute(CInteractionContext& Context, bool Enqueue) const override;
};

}
