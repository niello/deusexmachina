#pragma once
#include <Game/Interaction/Interaction.h>

// Lockpick a locked object, e.g. door or container

namespace DEM::RPG
{

class CLockpickInteraction : public Game::CInteraction
{
public:

	CLockpickInteraction(std::string_view CursorImage = {});

	virtual bool      IsAvailable(const Game::CInteractionContext& Context) const;
	virtual bool      IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const override;
	virtual ESoftBool NeedMoreTargets(const Game::CInteractionContext& Context) const override;
	virtual bool      Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue) const override;
};

}
