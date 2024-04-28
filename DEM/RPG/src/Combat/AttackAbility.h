#pragma once
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Events/Signal.h>

// Attack a destructible object with the current attack (weapon, natural or some other)

namespace DEM::RPG
{

class CAttackAbilityInstance : public Game::CAbilityInstance
{
public:

	Events::CConnection HitConn;

	using CAbilityInstance::CAbilityInstance;
};

class CAttackAbility : public Game::CAbility
{
protected:

	virtual Game::PAbilityInstance CreateInstance(const Game::CInteractionContext& Context) const override;

public:

	CAttackAbility(std::string_view CursorImage = {});

	virtual bool                IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const;
	virtual bool                IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const override;
	virtual ESoftBool           NeedMoreTargets(const Game::CInteractionContext& Context) const override;
	virtual bool                Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const override;

	virtual bool                GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const override;
	virtual bool                GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const override;
	virtual void                OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual Game::EActionStatus OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const override;
	virtual void                OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const override;
};

}
