#include <Scripting/ScriptServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Core/CoreServer.h>

// Script Interface for Time manager

namespace SI
{
using namespace Scripting;

int Time_CreateTimer(lua_State* l)
{
	//args: Name, Time, [Loop, Event, TimeSrc]

	int ArgCount = lua_gettop(l);

	if (ArgCount < 2 || !lua_isstring(l, 1) || !lua_isnumber(l, 2))
	{
		lua_settop(l, 0); //???need to call manually?
		lua_pushboolean(l, false);
		return 1;
	}

	bool Loop = (ArgCount > 2) ? lua_toboolean(l, 3) != 0 : false;
	CStrID Event = CStrID((ArgCount > 3) ? lua_tostring(l, 4) : "OnTimer");
	CStrID Src = (ArgCount > 4) ? CStrID(lua_tostring(l, 5)) : CStrID::Empty;

	bool Result = CoreSrv->CreateNamedTimer(CStrID(lua_tostring(l, 1)), (float)lua_tonumber(l, 2), Loop, Event, Src);

	lua_pushboolean(l, Result);
	return 1;
}
//---------------------------------------------------------------------

int Time_PauseTimer(lua_State* l)
{
	//args: Name, [Pause]
	
	int ArgCount = lua_gettop(l);

	if (ArgCount < 1 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	CoreSrv->PauseNamedTimer(CStrID(lua_tostring(l, 1)), (ArgCount > 1) ? lua_toboolean(l, 2) != 0 : true);
	return 0;
}
//---------------------------------------------------------------------

int Time_DestroyTimer(lua_State* l)
{
	//args: Name
	
	int ArgCount = lua_gettop(l);

	if (ArgCount < 1 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	CoreSrv->DestroyNamedTimer(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

int Time_SetTimeScale(lua_State* l)
{
	//args: TimeSpeed

	int ArgCount = lua_gettop(l);

	if (ArgCount < 1 || !lua_isnumber(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	CoreSrv->SetTimeScale((float)lua_tonumber(l, 1));

	return 0;
}
//---------------------------------------------------------------------

bool RegisterTime()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 4);
	ScriptSrv->ExportCFunction("CreateTimer", Time_CreateTimer);
	ScriptSrv->ExportCFunction("PauseTimer", Time_PauseTimer);
	ScriptSrv->ExportCFunction("DestroyTimer", Time_DestroyTimer);
	ScriptSrv->ExportCFunction("SetTimeScale", Time_SetTimeScale);
	lua_setglobal(l, "Time");

	OK;
}
//---------------------------------------------------------------------

}