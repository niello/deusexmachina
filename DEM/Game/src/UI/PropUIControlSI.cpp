#include "PropUIControl.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <Game/Entity.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

int CPropUIControl_EnableUI(lua_State* l)
{
	//args: EntityScriptObject's this table, [bool enable = true]
	SETUP_ENT_SI_ARGS(1);

	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	pCtl->Enable((ArgCount > 1) ? !!lua_toboolean(l, 2) : true);
	return 0;
}
//---------------------------------------------------------------------

int CPropUIControl_IsUIEnabled(lua_State* l)
{
	//args: EntityScriptObject's this table
	SETUP_ENT_SI_ARGS(1);

	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	lua_pushboolean(l, pCtl->IsEnabled());
	return 1;
}
//---------------------------------------------------------------------

int CPropUIControl_SetUIName(lua_State* l)
{
	//args: EntityScriptObject's this table, name
	SETUP_ENT_SI_ARGS(2);

	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	pCtl->SetUIName(lua_tostring(l, 2));
	return 0;
}
//---------------------------------------------------------------------

int CPropUIControl_AddActionHandler(lua_State* l)
{
	//args: EntityScriptObject's this table, action ID, action UI name, handler function name, [priority]
	SETUP_ENT_SI_ARGS(4);

	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	lua_pushboolean(l,
		pCtl->AddActionHandler(
			CStrID(lua_tostring(l, 2)),
			lua_tostring(l, 3),
			lua_tostring(l, 4),
			(ArgCount > 3) ? (int)lua_tointeger(l, 5) : CPropUIControl::Priority_Default));
	return 1;
}
//---------------------------------------------------------------------

int CPropUIControl_RemoveActionHandler(lua_State* l)
{
	//args: EntityScriptObject's this table, action ID
	SETUP_ENT_SI_ARGS(2);
	
	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	pCtl->RemoveActionHandler(CStrID(lua_tostring(l, 2)));
	return 0;
}
//---------------------------------------------------------------------

void CPropUIControl::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ExportCFunction("EnableUI", CPropUIControl_EnableUI);
	ScriptSrv->ExportCFunction("IsUIEnabled", CPropUIControl_IsUIEnabled);
	ScriptSrv->ExportCFunction("SetUIName", CPropUIControl_SetUIName);
	ScriptSrv->ExportCFunction("AddUIActionHandler", CPropUIControl_AddActionHandler);
	ScriptSrv->ExportCFunction("RemoveUIActionHandler", CPropUIControl_RemoveActionHandler);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropUIControl::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ClearField("EnableUI");
	ScriptSrv->ClearField("IsUIEnabled");
	ScriptSrv->ClearField("SetUIName");
	ScriptSrv->ClearField("AddUIActionHandler");
	ScriptSrv->ClearField("RemoveUIActionHandler");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}
