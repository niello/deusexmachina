#include "PropInventory.h"

#include <Items/Item.h>
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

int CPropInventory_AddItem(lua_State* l)
{
	//args: EntityScriptObject's this table, Item ID, [Count = 1]
	SETUP_ENT_SI_ARGS(2);

	int Count = (ArgCount > 2 && lua_isnumber(l, 3)) ? lua_tointeger(l, 3) : 1;
	if (Count > 0)
	{
		lua_pushboolean(l,
			This->GetEntity()->GetProperty<CPropInventory>()->AddItem(CStrID(lua_tostring(l, 2)), Count));
		return 1;
	}
	return 0; //!!!how to determine failure reason in script?!
}
//---------------------------------------------------------------------

int CPropInventory_RemoveItem(lua_State* l)
{
	//args: EntityScriptObject's this table, Item ID, [Count = 1], [AsManyAsCan = false]
	SETUP_ENT_SI_ARGS(2);

	int Count = (ArgCount > 2 && lua_isnumber(l, 3)) ? lua_tointeger(l, 3) : 1;
	bool AsManyAsCan = (ArgCount > 3 && lua_isboolean(l, 4)) ? lua_toboolean(l, 4) != 0 : false;
	if (Count > 0)
	{
		lua_pushboolean(l,
			This->GetEntity()->GetProperty<CPropInventory>()->RemoveItem(CStrID(lua_tostring(l, 2)),
				Count, AsManyAsCan));
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------

int CPropInventory_HasItem(lua_State* l)
{
	//args: EntityScriptObject's this table, Item ID, [Count = 1]
	SETUP_ENT_SI_ARGS(2);

	int Count = (ArgCount > 2 && lua_isnumber(l, 3)) ? lua_tointeger(l, 3) : 1;
	if (Count > 0)
	{
		lua_pushboolean(l,
			This->GetEntity()->GetProperty<CPropInventory>()->HasItem(CStrID(lua_tostring(l, 2)), Count));
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------

void CPropInventory::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ExportCFunction("AddItem", CPropInventory_AddItem);
	ScriptSrv->ExportCFunction("RemoveItem", CPropInventory_RemoveItem);
	ScriptSrv->ExportCFunction("HasItem", CPropInventory_HasItem);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropInventory::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ClearField("AddItem");
	ScriptSrv->ClearField("RemoveItem");
	ScriptSrv->ClearField("HasItem");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}