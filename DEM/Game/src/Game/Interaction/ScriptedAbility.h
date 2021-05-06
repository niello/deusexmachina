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

	sol::function _FnGetInteractionPoint;
	sol::function _FnGetFacingParams;
	sol::function _FnOnStart;
	sol::function _FnOnUpdate;
	sol::function _FnOnEnd;

public:

	CScriptedAbility(const sol::table& Table);

	virtual bool          IsAvailable(const CInteractionContext& Context) const override;
	virtual bool          IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual ESoftBool     NeedMoreTargets(const CInteractionContext& Context) const override;
	virtual bool          Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const override;

	virtual bool          GetZones(std::vector<const CZone*>& Out) const override;
	virtual bool          GetFacingParams(CFacingParams& Out) const override;
	virtual void          OnStart() const override; // ActorEntity, TargetEntity
	virtual EActionStatus OnUpdate() const override; // ActorEntity, TargetEntity, AbilityInstance - instance everywhere?!
	virtual void          OnEnd() const override; // ActorEntity, TargetEntity, Status
};

}
