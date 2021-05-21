#pragma once
#include <Game/Interaction/Interaction.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CMoveInteraction : public CInteraction
{
public:

	CMoveInteraction(std::string_view CursorImage = {});

	virtual bool      IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual ESoftBool NeedMoreTargets(const CInteractionContext& Context) const override;
	virtual bool      Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue, bool PushChild) const override;
};

}
