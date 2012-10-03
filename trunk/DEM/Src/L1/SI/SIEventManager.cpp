#include <Scripting/ScriptServer.h>
#include <Scripting/ScriptObject.h>
#include <Scripting/EventHandlerScript.h>
#include <Events/EventManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for Event manager

namespace SI
{
using namespace Scripting;
using namespace Events;

int EventMgr_Subscribe(lua_State* l)
{
	//args: ScriptObject's this table or nil, event name, [func name = event name]
	
	if (lua_isnil(l, 1))
	{
		//!!!if arg 1 is nil subscribe global function!
		lua_settop(l, 0);
		return 0;
	}
	else return CScriptObject_SubscribeEvent(l);
}
//---------------------------------------------------------------------

int EventMgr_Unsubscribe(lua_State* l)
{
	//args: ScriptObject's this table or nil, event name, [func name = event name]

	if (lua_isnil(l, 1))
	{
		//!!!if arg 1 is nil unsubscribe global function!
		lua_settop(l, 0);
		return 0;
	}
	else return CScriptObject_UnsubscribeEvent(l);
}
//---------------------------------------------------------------------

int EventMgr_FireEvent(lua_State* l)
{
	//args: event name, [parameter or parameter table]

	int ArgCount = lua_gettop(l);
		
	if (ArgCount < 1 || ArgCount > 2 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0);
		return 0;
	}

	PParams Params;
	if (ArgCount > 1 && !lua_isnil(l, 2))
	{
		CData Data;
		ScriptSrv->LuaStackToData(Data, 2, l);
		if (lua_istable(l, 2)) Params = (PParams)Data;
		else if (Data.IsValid())
		{
			//???for all args read and push into array?
			Params = n_new(CParams(1));
			Params->Set(CStrID::Empty, Data);
		}
	}

	EventMgr->FireEvent(CStrID(lua_tostring(l, 1)), Params);
	return 0;
}
//---------------------------------------------------------------------

bool RegisterEventManager()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 3);
	ScriptSrv->ExportCFunction("Subscribe", EventMgr_Subscribe);
	ScriptSrv->ExportCFunction("Unsubscribe", EventMgr_Unsubscribe);
	ScriptSrv->ExportCFunction("FireEvent", EventMgr_FireEvent);
	lua_setglobal(l, "EventMgr");

	OK;
}
//---------------------------------------------------------------------

}