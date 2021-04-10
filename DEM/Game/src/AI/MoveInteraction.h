#pragma once
#include <Game/Interaction/Interaction.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CMoveInteraction : public CInteraction
{
public:

	CMoveInteraction(std::string_view CursorImage = {});

	virtual bool Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const override;
};

}
