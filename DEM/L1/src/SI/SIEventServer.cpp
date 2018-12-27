#include <Scripting/ScriptServer.h>
#include <Scripting/ScriptObject.h>
#include <Scripting/EventHandlerScript.h>
#include <Events/EventServer.h>

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

int EventSrv_Subscribe(lua_State* l)
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

int EventSrv_Unsubscribe(lua_State* l)
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

int EventSrv_FireEvent(lua_State* l)
{
	//args: event name, [parameter or parameter table]

	int ArgCount = lua_gettop(l);
		
	if (ArgCount < 1 || ArgCount > 2 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0);
		return 0;
	}

	Data::PParams Params;
	if (ArgCount > 1 && !lua_isnil(l, 2))
	{
		Data::CData Data;
		ScriptSrv->LuaStackToData(Data, 2); //??? l as arg?
		if (lua_istable(l, 2)) Params = (Data::PParams)Data;
		else if (Data.IsValid())
		{
			//???for all args read and push into array?
			Params = n_new(Data::CParams(1));
			Params->Set(CStrID::Empty, Data);
		}
	}

	EventSrv->FireEvent(CStrID(lua_tostring(l, 1)), Params);
	return 0;
}
//---------------------------------------------------------------------

bool RegisterEventServer()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 3);
	ScriptSrv->ExportCFunction("Subscribe", EventSrv_Subscribe);
	ScriptSrv->ExportCFunction("Unsubscribe", EventSrv_Unsubscribe);
	ScriptSrv->ExportCFunction("FireEvent", EventSrv_FireEvent);
	lua_setglobal(l, "EventSrv");

	OK;
}
//---------------------------------------------------------------------

}