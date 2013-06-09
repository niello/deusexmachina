#include "PropUIControl.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/EntityManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

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
			(ArgCount > 3) ? lua_tointeger(l, 5) : CPropUIControl::DEFAULT_PRIORITY));
	return 1;
}
//---------------------------------------------------------------------

int CPropUIControl_RemoveActionHandler(lua_State* l)
{
	//args: EntityScriptObject's this table, action ID
	SETUP_ENT_SI_ARGS(4);
	
	CPropUIControl* pCtl = This->GetEntity()->GetProperty<CPropUIControl>();
	pCtl->RemoveActionHandler(CStrID(lua_tostring(l, 2)));
	return 0;
}
//---------------------------------------------------------------------

bool CPropUIControl::ExposeSI(const Events::CEventBase& Event)
{
	ScriptSrv->ExportCFunction("AddUIActionHandler", CPropUIControl_AddActionHandler);
	ScriptSrv->ExportCFunction("RemoveUIActionHandler", CPropUIControl_RemoveActionHandler);
	OK;
}
//---------------------------------------------------------------------

} // namespace Prop
