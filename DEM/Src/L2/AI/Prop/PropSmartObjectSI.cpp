#include "PropSmartObject.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/Entity.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Properties
{
using namespace Scripting;

int CPropSmartObject_SetState(lua_State* l)
{
	//args: EntityScriptObject's this table, State ID
	SETUP_ENT_SI_ARGS(2);
	This->GetEntity()->GetProperty<CPropSmartObject>()->SetState(CStrID(lua_tostring(l, 2)));
	return 0;
}
//---------------------------------------------------------------------

int CPropSmartObject_GetState(lua_State* l)
{
	//args: EntityScriptObject's this table
	//ret:  State ID
	SETUP_ENT_SI_ARGS(1);
	lua_pushstring(l, This->GetEntity()->GetProperty<CPropSmartObject>()->GetCurrState().CStr());
	return 1;
}
//---------------------------------------------------------------------

int CPropSmartObject_IsInState(lua_State* l)
{
	//args: EntityScriptObject's this table, State ID
	//ret:  bool is in state
	SETUP_ENT_SI_ARGS(1);
	CStrID State = This->GetEntity()->GetProperty<CPropSmartObject>()->GetCurrState();
	lua_pushboolean(l, State == lua_tostring(l, 2));
	return 1;
}
//---------------------------------------------------------------------

int CPropSmartObject_EnableAction(lua_State* l)
{
	//args: EntityScriptObject's this table, Action ID, [Enabled = true]
	SETUP_ENT_SI_ARGS(2);
	bool Enable = (ArgCount > 2) ? !!lua_toboolean(l, 3) : true;
	This->GetEntity()->GetProperty<CPropSmartObject>()->EnableAction(CStrID(lua_tostring(l, 2)), Enable);
	return 0;
}
//---------------------------------------------------------------------

int CPropSmartObject_IsActionEnabled(lua_State* l)
{
	//args: EntityScriptObject's this table, Action ID
	//ret:  bool is enabled
	SETUP_ENT_SI_ARGS(2);
	lua_pushboolean(l, This->GetEntity()->GetProperty<CPropSmartObject>()->IsActionEnabled(CStrID(lua_tostring(l, 2))));
	return 1;
}
//---------------------------------------------------------------------

bool CPropSmartObject::ExposeSI(const CEventBase& Event)
{
	ScriptSrv->ExportCFunction("SetState", CPropSmartObject_SetState);
	ScriptSrv->ExportCFunction("GetState", CPropSmartObject_GetState);
	ScriptSrv->ExportCFunction("IsInState", CPropSmartObject_IsInState);
	ScriptSrv->ExportCFunction("EnableAction", CPropSmartObject_EnableAction);
	ScriptSrv->ExportCFunction("IsActionEnabled", CPropSmartObject_IsActionEnabled);
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties