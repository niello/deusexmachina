#include "PropActorBrain.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <AI/AIServer.h>
#include <AI/Behaviour/ActionSequence.h>
#include <AI/Movement/Actions/ActionGotoPosition.h>
#include <AI/Movement/Actions/ActionGotoTarget.h>
#include <AI/SmartObj/Actions/ActionGotoSmartObj.h>
#include <AI/SmartObj/Actions/ActionUseSmartObj.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

int CPropActorBrain_EnqueueTask(lua_State* l)
{
	//args: EntityScriptObject's this table, plan desc table, [Relevance = Absolute, FailOnInterruption = false, ClearQueueOnFailure = false]
	SETUP_ENT_SI_ARGS(2);

	Data::CData PlanDescData;
	ScriptSrv->LuaStackToData(PlanDescData, 2);
	if (!PlanDescData.IsA<Data::PParams>()) return 0;

	CTask Task;
	Task.Plan = AI::CAIServer::CreatePlanFromDesc(PlanDescData.GetValue<Data::PParams>());
	Task.Relevance = ArgCount >= 3 ? (float)lua_tonumber(l, 3) : AI::Relevance_Absolute;
	Task.FailOnInterruption = ArgCount >= 4 && lua_toboolean(l, 4) != 0;
	Task.ClearQueueOnFailure = ArgCount >= 5 && lua_toboolean(l, 5) != 0;
	This->GetEntity()->GetProperty<Prop::CPropActorBrain>()->EnqueueTask(Task);
	return 0;
}
//---------------------------------------------------------------------

int CPropActorBrain_ClearTaskQueue(lua_State* l)
{
	//args: EntityScriptObject's this table
	SETUP_ENT_SI_ARGS(1);
	This->GetEntity()->GetProperty<Prop::CPropActorBrain>()->ClearTaskQueue();
	return 0;
}
//---------------------------------------------------------------------

int CPropActorBrain_DoAction(lua_State* l)
{
	//args: EntityScriptObject's this table, SO ID, Action ID, [Relevance = Absolute, FailOnInterruption = false, ClearQueueOnFailure = false]
	SETUP_ENT_SI_ARGS(3);

	CStrID TargetID = CStrID(lua_tostring(l, 2));
	CStrID ActionID = CStrID(lua_tostring(l, 3));

	CTask Task;

	PActionGotoSmartObj ActGoto = n_new(CActionGotoSmartObj);
	ActGoto->Init(TargetID, ActionID);

	PActionUseSmartObj ActUse = n_new(CActionUseSmartObj);
	ActUse->Init(TargetID, ActionID);

	PActionSequence Plan = n_new(CActionSequence);
	Plan->AddChild(ActGoto);
	Plan->AddChild(ActUse);

	Task.Plan = Plan;
	Task.Relevance = ArgCount >= 4 ? (float)lua_tonumber(l, 4) : AI::Relevance_Absolute;
	Task.FailOnInterruption = ArgCount >= 5 && lua_toboolean(l, 5) != 0;
	Task.ClearQueueOnFailure = ArgCount >= 6 && lua_toboolean(l, 6) != 0;

	This->GetEntity()->GetProperty<Prop::CPropActorBrain>()->EnqueueTask(Task);
	return 0;
}
//---------------------------------------------------------------------

int CPropActorBrain_Go(lua_State* l)
{
	//args1: EntityScriptObject's this table, X, Y, Z, [Relevance = Absolute, FailOnInterruption = false, ClearQueueOnFailure = false]
	//args2: EntityScriptObject's this table, Target entity ID, [Relevance = Absolute, FailOnInterruption = false, ClearQueueOnFailure = false]
	SETUP_ENT_SI_ARGS(2);

	//!!!get EMovementType MvmtType from params, set to goto action?!
	//must be in a base CActionGoto class

	CTask Task;

	int NextArg;
	if (ArgCount >= 4 && lua_type(l, 2) == LUA_TNUMBER)
	{
		// XYZ case
		PActionGotoPosition Action = n_new(CActionGotoPosition);
		Action->Init(vector3((float)lua_tonumber(l, 2), (float)lua_tonumber(l, 3), (float)lua_tonumber(l, 4)));
		Task.Plan = Action;
		NextArg = 5;
	}
	else
	{
		// Target entity case
		PActionGotoTarget Action = n_new(CActionGotoTarget);
		Action->Init(CStrID(lua_tostring(l, 2)));
		Task.Plan = Action;
		NextArg = 3;
	}

	Task.Relevance = ArgCount >= NextArg ? (float)lua_tonumber(l, NextArg) : AI::Relevance_Absolute;
	Task.FailOnInterruption = ArgCount > NextArg + 1 && lua_toboolean(l, NextArg + 1) != 0;
	Task.ClearQueueOnFailure = ArgCount > NextArg + 2 && lua_toboolean(l, NextArg + 2) != 0;

	This->GetEntity()->GetProperty<Prop::CPropActorBrain>()->EnqueueTask(Task);
	return 0;
}
//---------------------------------------------------------------------

int CPropActorBrain_AbortCurrAction(lua_State* l)
{
	//args: EntityScriptObject's this table, [Result = Success]
	SETUP_ENT_SI_ARGS(1);
	This->GetEntity()->GetProperty<CPropActorBrain>()->AbortCurrAction((ArgCount > 1) ? (DWORD)lua_tonumber(l, 2) : Success);
	return 0;
}
//---------------------------------------------------------------------

void CPropActorBrain::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("EnqueueTask", CPropActorBrain_EnqueueTask);
	ScriptSrv->ExportCFunction("ClearTaskQueue", CPropActorBrain_ClearTaskQueue);
	ScriptSrv->ExportCFunction("DoAction", CPropActorBrain_DoAction);
	ScriptSrv->ExportCFunction("Go", CPropActorBrain_Go);
	ScriptSrv->ExportCFunction("AbortCurrAction", CPropActorBrain_AbortCurrAction);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropActorBrain::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("EnqueueTask");
	ScriptSrv->ClearField("ClearTaskQueue");
	ScriptSrv->ClearField("DoAction");
	ScriptSrv->ClearField("Go");
	ScriptSrv->ClearField("AbortCurrAction");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}