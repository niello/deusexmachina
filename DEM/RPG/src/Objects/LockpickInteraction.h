#pragma once
#include <Game/Interaction/Interaction.h>

// Lockpick a locked object, e.g. door or container

namespace DEM::RPG
{

class CLockpickInteraction : public Game::CInteraction
{
public:

	CLockpickInteraction(std::string_view CursorImage = {});

	virtual bool Execute(Game::CInteractionContext& Context, bool Enqueue) const override;
};

}
