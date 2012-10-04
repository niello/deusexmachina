#include "EventHandlerScript.h"

#include "ScriptObject.h"
#include "ScriptServer.h"
#include <Events/EventDispatcher.h> //!!!now for PSub only!

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Events
{

bool CEventHandlerScript::operator()(const CEventBase& Event)
{
	const CEvent& e = (const CEvent&)Event;

	if (e.Params.isvalid() && e.Params->GetCount())
	{
		//!!!now only parametrized events are supported!

		lua_State* l = ScriptSrv->GetLuaState();

		//lua_getglobal(l, "CurrEvent");
		//!!!FIXME!
		//!!!not guaranteed that curr CEventBase was not reused in the same frame for other event!
		//???use some event UID cleared to 0 each frame & compare by ID?
		//or at first conversion set some flag into event params?
		//???may be USE STATIC COUNTER and update on value changed?
		//if (lua_isnil(l, -1) || lua_touserdata(l, -1) != &Event)
		//{
			if (ScriptSrv->DataToLuaStack(((const CEvent&)Event).Params) == 1)
			{
				lua_pushstring(l, ((const CEvent&)Event).GetID().ID);
				lua_setfield(l, -2, "EventName");
				lua_setglobal(l, "CurrEventParams");
			}
			//lua_pushlightuserdata(l, (void*)&Event);
			//lua_setglobal(l, "CurrEvent");
		//}
		//lua_pop(l, 1);

		if (pObject) pObject->RunFunction(Func.Get(), "CurrEventParams");
		//!!!else ScriptSrv->RunFunction(Func.Get());! (global func call)
	}
	else
	{
		if (pObject) pObject->RunFunction(Func.Get());
		//!!!else ScriptSrv->RunFunction(Func.Get());! (global func call)
	}

	OK; //!!!real return value needed!
}
//---------------------------------------------------------------------

}
