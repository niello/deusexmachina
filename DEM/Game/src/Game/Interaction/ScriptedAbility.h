#pragma once
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Scripting/SolGame.h>

// Actor's ability implemented fully in script

namespace DEM::Game
{

class CScriptedAbilityInstance : public CAbilityInstance
{
public:

	sol::table Custom;

	using CAbilityInstance::CAbilityInstance;
};

class CScriptedAbility : public CAbility
{
protected:

	sol::state_view _Lua;

	sol::function _FnIsAvailable;
	sol::function _FnIsTargetValid;
	sol::function _FnPrepare;

	sol::function _FnGetZones;
	sol::function _FnGetFacingParams;
	sol::function _FnOnStart;
	sol::function _FnOnUpdate;
	sol::function _FnOnEnd;

	virtual PAbilityInstance CreateInstance(const CInteractionContext& Context) const override;

public:

	CScriptedAbility(sol::state_view Lua, const sol::table& Table);

	virtual bool          IsAvailable(const CGameSession& Session, const CInteractionContext& Context) const override;
	virtual bool          IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual bool          Execute(CGameSession& Session, CInteractionContext& Context) const override;

	virtual bool          GetZones(const Game::CGameSession& Session, const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const override;
	virtual bool          GetFacingParams(const Game::CGameSession& Session, const CAbilityInstance& Instance, CFacingParams& Out) const override;
	virtual void          OnStart(Game::CGameSession& Session, CAbilityInstance& Instance) const override;
	virtual AI::ECommandStatus OnUpdate(Game::CGameSession& Session, CAbilityInstance& Instance) const override;
	virtual void          OnEnd(Game::CGameSession& Session, CAbilityInstance& Instance, AI::ECommandStatus Status) const override;
};

}
