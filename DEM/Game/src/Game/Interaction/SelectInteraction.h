#pragma once
#include <Game/Interaction/Interaction.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CSelectInteraction : public CInteraction
{
public:

	CSelectInteraction(std::string_view CursorImage = {});

	virtual bool      IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual bool      Execute(CGameSession& Session, CInteractionContext& Context) const override;
};

}
