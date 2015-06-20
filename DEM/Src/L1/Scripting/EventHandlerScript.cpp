#include "EventHandlerScript.h"

#include <Scripting/ScriptObject.h>
#include <Scripting/ScriptServer.h>
#include <Events/EventServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Events
{

bool CEventHandlerScript::Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event)
{
	//???where do we check isn't it native?
	const CEvent& e = (const CEvent&)Event;

	if (e.Params.IsValid() && e.Params->GetCount())
	{
		// We cache e.Params in CurrEventParams table for multiple handlers to access
		// without re-converting params each time. Here we check actuality of the cache.
		lua_State* l = ScriptSrv->GetLuaState();
		lua_getglobal(l, "CurrEventParams");
		bool CacheIsActual = lua_istable(l, -1);
		if (CacheIsActual)
		{
			lua_pushstring(l, "_EV_UID");
			lua_rawget(l, -2);
			CacheIsActual = (lua_type(l, -1) == LUA_TNUMBER && lua_tointeger(l, -1) == EventSrv->GetFiredEventsCount());
			lua_pop(l, 2);
		}
		else lua_pop(l, 1);

		// Update params cache. DataToLuaStack returns table, and we add a couple of fields into it. 
		if (!CacheIsActual && ScriptSrv->DataToLuaStack(e.Params) == 1)
		{
			n_assert_dbg(lua_istable(l, -1));
			lua_pushstring(l, "EventID");
			lua_pushstring(l, e.GetID().ID);
			lua_rawset(l, -3);
			lua_pushstring(l, "_EV_UID");
			lua_pushinteger(l, EventSrv->GetFiredEventsCount());
			lua_rawset(l, -3);
			lua_setglobal(l, "CurrEventParams");
		}

		if (pObject) pObject->RunFunction(Func.CStr(), "CurrEventParams");
		//!!!else ScriptSrv->RunFunction(Func.CStr(), "CurrEventParams");! (global func call)
	}
	else
	{
		if (pObject) pObject->RunFunction(Func.CStr());
		//!!!else ScriptSrv->RunFunction(Func.CStr());! (global func call)
	}

	OK; //!!!real return value needed!
}
//---------------------------------------------------------------------

}
