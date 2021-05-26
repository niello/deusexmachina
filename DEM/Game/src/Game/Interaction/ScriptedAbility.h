#pragma once
#include <Game/Interaction/Ability.h>
#include <sol/sol.hpp>

// Actor's ability implemented fully in script

//???need CScriptedAbilityInstance with Lua table for arbitrary state?

namespace DEM::Game
{

class CScriptedAbility : public CAbility
{
protected:

	sol::function _FnIsAvailable;
	sol::function _FnIsTargetValid;
	sol::function _FnNeedMoreTargets;
	sol::function _FnPrepare;

	sol::function _FnGetZones;
	sol::function _FnGetFacingParams;
	sol::function _FnOnStart;
	sol::function _FnOnUpdate;
	sol::function _FnOnEnd;

public:

	CScriptedAbility(const sol::table& Table);

	virtual bool          IsAvailable(const CInteractionContext& Context) const override;
	virtual bool          IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual ESoftBool     NeedMoreTargets(const CInteractionContext& Context) const override;
	virtual bool          Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue, bool PushChild) const override;

	virtual bool          GetZones(const Game::CGameSession& Session, const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const override;
	virtual bool          GetFacingParams(const Game::CGameSession& Session, const CAbilityInstance& Instance, CFacingParams& Out) const override;
	virtual void          OnStart(Game::CGameSession& Session, CAbilityInstance& Instance) const override;
	virtual EActionStatus OnUpdate(Game::CGameSession& Session, CAbilityInstance& Instance) const override;
	virtual void          OnEnd(Game::CGameSession& Session, CAbilityInstance& Instance, EActionStatus Status) const override;
};

}
