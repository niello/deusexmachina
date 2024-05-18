#pragma once
#include <Game/Interaction/Interaction.h>
#include <Scripting/SolGame.h>

// Interaction implemented fully in script

//!!!FIXME: no need in Session as arg! Can't use other session than that which owns Lua objects!!!

namespace DEM::Game
{

class CScriptedInteraction : public CInteraction
{
protected:

	sol::function _FnIsAvailable;
	sol::function _FnIsTargetValid;
	sol::function _FnExecute;

public:

	CScriptedInteraction(const sol::table& Table);

	virtual bool      IsAvailable(const CGameSession& Session, const CInteractionContext& Context) const override;
	virtual bool      IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const override;
	virtual bool      Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue, bool PushChild) const override;
};

}
