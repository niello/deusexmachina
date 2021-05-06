#include "ScriptedAbility.h"
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::Game
{

// TODO: common utility function?!
// FIXME: DUPLICATED CODE! See CScriptedAbility!
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

	if (Result.get_type() == sol::type::nil || !Result) return false;

	return true;
}
//---------------------------------------------------------------------

CScriptedAbility::CScriptedAbility(const sol::table& Table)
{
	_FnIsAvailable = Table.get<sol::function>("IsAvailable");
	_FnIsTargetValid = Table.get<sol::function>("IsTargetValid");
	_FnNeedMoreTargets = Table.get<sol::function>("NeedMoreTargets");
	_FnPrepare = Table.get<sol::function>("Prepare");

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
	// Call Prepare - filter actors, adjust ability params (like +2 bonus in skill for each helping actor)
	// Fail if no actors left
	// Enqueue ExecuteAbility actions into filtered actors' queues, create AbilityInstance for each one

	NOT_IMPLEMENTED;
	return false;

	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;
	//auto pSOComponent = pWorld->FindComponent<CSmartObjectComponent>(Context.Targets[0].Entity);
	//if (!pSOComponent || !pSOComponent->Asset) return false;
	//CSmartObject* pSOAsset = pSOComponent->Asset->GetObject<CSmartObject>();
	//if (!pSOAsset) return false;

	auto pQueue = pWorld->FindComponent<CActionQueueComponent>(Context.Actors[0]);
	if (!pQueue) return false;

	if (!Enqueue) pQueue->Reset();
	//pQueue->EnqueueAction<ExecuteAbility>(Context.Interaction, Context.Targets[0].Entity);

	return true;
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetZones(std::vector<const CZone*>& Out) const
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetFacingParams(CFacingParams& Out) const
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnStart() const
{
}
//---------------------------------------------------------------------

EActionStatus CScriptedAbility::OnUpdate() const
{
	NOT_IMPLEMENTED;

	/*
	auto UpdateResult = _FnOnUpdate(args);
	if (!UpdateResult.valid())
	{
		sol::error Error = UpdateResult;
		::Sys::Error(Error.what());
		return EActionStatus::Failed;
	}
	else if (UpdateResult.get_type() == sol::type::number)
	{
		// Enums are represented as numbers in Sol
		EActionStatus NewStatus = UpdateResult;
		if (NewStatus != EActionStatus::Active) return NewStatus;
	}
	else
	{
		//!!!TODO: fmtlib and variadic args in assertion macros!
		n_assert2_dbg(UpdateResult.get_type() == sol::type::none, ("Unexpected return type from SO lua OnUpdate" + AIState.CurrInteraction.ToString()).c_str());
	}
	*/

	return EActionStatus::Active;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnEnd() const
{
}
//---------------------------------------------------------------------

}
