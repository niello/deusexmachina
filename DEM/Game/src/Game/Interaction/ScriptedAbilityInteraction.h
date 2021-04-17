#pragma once
#include <Game/Interaction/Interaction.h>
#include <sol/sol.hpp>

// Scripted interaction that triggers an actor's ability

//!!!FIXME: no need in Session as arg! Can't use other session than that which owns Lua objects!!!
//???!!!store arbitrary ability?! ability - refcounted?! or registered by ID?

//!!!FIXME: too much duplicated logic with CScriptedInteraction!!!

namespace DEM::Game
{

class CScriptedAbilityInteraction : public CInteraction
{
protected:

	sol::function _FnIsAvailable;
	sol::function _FnIsTargetValid;
	sol::function _FnNeedMoreTargets;
	sol::function _FnPrepare;

public:

	CScriptedAbilityInteraction(const sol::table& Table /*, ability*/);

	virtual bool      IsAvailable(const CInteractionContext& Context) const override;
	virtual bool      IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual ESoftBool NeedMoreTargets(const CInteractionContext& Context) const override;
	virtual bool      Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const override;
};

}
