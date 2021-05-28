#pragma once
#include <Game/Interaction/Ability.h>

// Lockpick a locked object, e.g. door or container

namespace DEM::RPG
{

class CLockpickAbility : public Game::CAbility
{
public:

	CLockpickAbility(std::string_view CursorImage = {});

	virtual bool                IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const;
	virtual bool                IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const override;
	virtual ESoftBool           NeedMoreTargets(const Game::CInteractionContext& Context) const override;
	virtual bool                Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const override;

	virtual bool                GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const override;
	virtual void                OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual Game::EActionStatus OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual void                OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const override;
};

}
