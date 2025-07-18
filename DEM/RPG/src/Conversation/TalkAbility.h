#pragma once
#include <Game/Interaction/Ability.h>

// Start a conversation with a target if both actors can speak

namespace DEM::RPG
{

class CTalkAbility : public Game::CAbility
{
public:

	CTalkAbility(std::string_view CursorImage = {});

	virtual bool                IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const;
	virtual bool                IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const override;
	virtual bool                Execute(Game::CGameSession& Session, Game::CInteractionContext& Context) const override;

	virtual bool                GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const override;
	virtual bool                GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const override;
	virtual void                OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual AI::ECommandStatus  OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual void                OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, AI::ECommandStatus Status) const override;
};

}
