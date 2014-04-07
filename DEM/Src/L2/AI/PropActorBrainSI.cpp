#include "PropActorBrain.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <Game/EntityManager.h>
#include <AI/PropSmartObject.h>
#include <AI/Behaviour/ActionSequence.h>
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

int CPropActorBrain_DoAction(lua_State* l)
{
	//args: EntityScriptObject's this table, SO ID, Action ID, [Relevance = 1.f, ClearQueueOnFailure = false]
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

	if (ArgCount > 3) Task.Relevance = (float)lua_tonumber(l, 4);
	if (ArgCount > 4) Task.ClearQueueOnFailure = lua_toboolean(l, 5) != 0;

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

int CPropActorBrain_Go(lua_State* l)
{
	//args1: EntityScriptObject's this table, X, Y, Z
	//args2: EntityScriptObject's this table, Target entity ID
	SETUP_ENT_SI_ARGS(2);

	CTask Task;

	//!!!get MvmtType from params, set to goto action?!
	//vector3			Point;
	//EMovementType	MvmtType;
	//pActor->GetNavSystem().SetDestPoint(Point);
	//pActor->MvmtType = MvmtType;

	//if (ArgCount >= 4 && lua_type(l, 2) == LUA_TNUMBER)
	//{
	//	// XYZ case
	//	Task->Point.set((float)lua_tonumber(l, 2), (float)lua_tonumber(l, 3), (float)lua_tonumber(l, 4));
	//}
	//else
	//{
	//	// Target entity case //???smart obj?
	//	Game::CEntity* pTarget = EntityMgr->GetEntity(CStrID(lua_tostring(l, 2)));
	//	if (!pTarget) return 0;
	//	Task->Point = pTarget->GetAttr<matrix44>(CStrID("Transform")).Translation();
	//}

	return 0;
}
//---------------------------------------------------------------------

void CPropActorBrain::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("DoAction", CPropActorBrain_DoAction);
	ScriptSrv->ExportCFunction("AbortCurrAction", CPropActorBrain_AbortCurrAction);
	ScriptSrv->ExportCFunction("Go", CPropActorBrain_Go);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropActorBrain::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("DoAction");
	ScriptSrv->ClearField("AbortCurrAction");
	ScriptSrv->ClearField("Go");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}