#include "PropActorBrain.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/EntityManager.h>
#include <AI/Events/QueueTask.h>
#include <AI/PropSmartObject.h>
#include <AI/SmartObj/Tasks/TaskUseSmartObj.h>
#include <AI/Movement/Tasks/TaskGoto.h>

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
	//args: EntityScriptObject's this table, IAO ID, Action ID
	SETUP_ENT_SI_ARGS(3);

	Game::CEntity* pTarget = EntityMgr->GetEntity(CStrID(lua_tostring(l, 2)));
	if (!pTarget) return 0;

	CPropSmartObject* pSO = pTarget->GetProperty<CPropSmartObject>();
	if (!pSO) return 0;

	//!!!can call function instead of sending event!
	PTaskUseSmartObj Task = n_new(CTaskUseSmartObj);
	Task->SetSmartObj(pSO);
	Task->SetActionID(CStrID(lua_tostring(l, 3)));
	This->GetEntity()->FireEvent(Event::QueueTask(Task));

	return 0;
}
//---------------------------------------------------------------------

int CPropActorBrain_Go(lua_State* l)
{
	//args1: EntityScriptObject's this table, X, Y, Z, [Arrive distance = 0]
	//args2: EntityScriptObject's this table, Target entity ID, [Arrive distance = 0]
	SETUP_ENT_SI_ARGS(2);

	AI::PTaskGoto Task = n_new(AI::CTaskGoto);

	//!!!get from params!
	Task->MvmtType = This->GetEntity()->GetProperty<CPropActorBrain>()->MvmtType;

	if (ArgCount >= 4 && lua_isnumber(l, 2))
	{
		Task->Point.set((float)lua_tonumber(l, 2), (float)lua_tonumber(l, 3), (float)lua_tonumber(l, 4));
		Task->MinDistance = 
		Task->MaxDistance = (ArgCount > 4) ? (float)lua_tonumber(l, 5) : 0.f;
	}
	else
	{
		//???autodetect smart object & use optional Action arg?
		Game::CEntity* pTarget = EntityMgr->GetEntity(CStrID(lua_tostring(l, 2)));
		if (!pTarget) return 0;
		Task->Point = pTarget->GetAttr<matrix44>(CStrID("Transform")).Translation();
		Task->MinDistance = 
		Task->MaxDistance = (ArgCount > 2) ? (float)lua_tonumber(l, 3) : 0.f;
	}

	This->GetEntity()->FireEvent(Event::QueueTask(Task));

	return 0;
}
//---------------------------------------------------------------------

bool CPropActorBrain::ExposeSI(const Events::CEventBase& Event)
{
	ScriptSrv->ExportCFunction("DoAction", CPropActorBrain_DoAction);
	ScriptSrv->ExportCFunction("Go", CPropActorBrain_Go);
	OK;
}
//---------------------------------------------------------------------

} // namespace Prop