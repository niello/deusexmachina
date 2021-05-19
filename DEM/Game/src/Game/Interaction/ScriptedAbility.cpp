#include "ScriptedAbility.h"
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/Interaction/Zone.h>

namespace DEM::Game
{

// TODO: common utility function?!
// FIXME: DUPLICATED CODE! See CScriptedInteraction!
template<typename... TArgs>
static bool LuaCall(const sol::function& Fn, TArgs&&... Args)
{
	if (!Fn) return false;

	auto Result = Fn(std::forward<TArgs>(Args)...);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return false;
	}

	auto Type = Result.get_type();
	return Type != sol::type::nil && Type != sol::type::none && Result;
}
//---------------------------------------------------------------------

CScriptedAbility::CScriptedAbility(const sol::table& Table)
{
	_FnIsAvailable = Table.get<sol::function>("IsAvailable");
	_FnIsTargetValid = Table.get<sol::function>("IsTargetValid");
	_FnNeedMoreTargets = Table.get<sol::function>("NeedMoreTargets");
	_FnPrepare = Table.get<sol::function>("Prepare");

	_FnGetZones = Table.get<sol::function>("GetZones");
	_FnGetFacingParams = Table.get<sol::function>("GetFacingParams");
	_FnOnStart = Table.get<sol::function>("OnStart");
	_FnOnUpdate = Table.get<sol::function>("OnUpdate");
	_FnOnEnd = Table.get<sol::function>("OnEnd");

	_CursorImage = Table.get<std::string>("CursorImage"); //???to method? pass target index?
}
//---------------------------------------------------------------------

bool CScriptedAbility::IsAvailable(const CInteractionContext& Context) const
{
	return !_FnIsAvailable || LuaCall(_FnIsAvailable, Context);
}
//---------------------------------------------------------------------

bool CScriptedAbility::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return LuaCall(_FnIsTargetValid, Index, Context);
}
//---------------------------------------------------------------------

ESoftBool CScriptedAbility::NeedMoreTargets(const CInteractionContext& Context) const
{
	auto Result = _FnNeedMoreTargets(Context);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return ESoftBool::False;
	}

	if (Result.get_type() == sol::type::boolean) return Result ? ESoftBool::True : ESoftBool::False;

	return Result;
}
//---------------------------------------------------------------------

bool CScriptedAbility::Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const
{
	if (Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;

	std::vector<HEntity> Actors;
	if (_FnPrepare)
	{
		// Filter actors, adjust ability params etc (like +2 bonus in skill for each helping actor)
		LuaCall(_FnPrepare, Actors);
	}
	else
	{
		Actors = Context.Actors;
	}

	UPTR Issued = 0;
	for (HEntity ActorID : Actors)
	{
		PAbilityInstance AbilityInstance(n_new(CAbilityInstance(*this)));
		AbilityInstance->Actor = ActorID;
		AbilityInstance->Source = Context.Source;
		AbilityInstance->Targets = Context.Targets;

		if (auto pQueue = pWorld->FindComponent<CActionQueueComponent>(ActorID))
		{
			if (!Enqueue) pQueue->Reset();
			pQueue->EnqueueAction<ExecuteAbility>(std::move(AbilityInstance));
			++Issued;
		}
	}

	return !!Issued;
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetZones(std::vector<const CZone*>& Out) const
{
	return LuaCall(_FnGetZones, Out);
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetFacingParams(CFacingParams& Out) const
{
	if (!_FnGetFacingParams) return false;
	LuaCall(_FnGetFacingParams, Out);
	return Out.Mode != EFacingMode::None;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnStart() const
{
	LuaCall(_FnOnStart);
}
//---------------------------------------------------------------------

EActionStatus CScriptedAbility::OnUpdate() const
{
	auto UpdateResult = _FnOnUpdate(/*args*/);
	if (!UpdateResult.valid())
	{
		sol::error Error = UpdateResult;
		::Sys::Error(Error.what());
		return EActionStatus::Failed;
	}
	else if (UpdateResult.get_type() != sol::type::number)
	{
		// Enums are represented as numbers in Sol. Also allow to return nil as an alias for Active.
		//!!!TODO: fmtlib and variadic args in assertion macros!
		n_assert2_dbg(UpdateResult.get_type() == sol::type::none, "Unexpected return type from lua OnUpdate");
		return EActionStatus::Active;
	}

	return UpdateResult;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnEnd(EActionStatus Status) const
{
	LuaCall(_FnOnEnd, Status);
}
//---------------------------------------------------------------------

}
