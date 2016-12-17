#include <Scripting/ScriptServer.h>
#include <StdDEM.h>
#include <Math/Math.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for global functions and constants

namespace SI
{

int Print(lua_State* l)
{
	//???concat args if > 1?
	if (lua_isboolean(l, 1)) Sys::Log(lua_toboolean(l, 1) == 0 ? "false" : "true");
	else Sys::Log(lua_tostring(l, 1));
	return 0;
}
//---------------------------------------------------------------------

int RandomInt(lua_State* l)
{
	// Args: min, max
	if (lua_gettop(l) < 2) return 0;
	lua_pushnumber(l, Math::RandomU32(lua_tointeger(l, 1), lua_tointeger(l, 2)));
	return 1;
}
//---------------------------------------------------------------------

int RandomFloat(lua_State* l)
{
	// Args: [min, max]
	int ArgCount = lua_gettop(l);
	if (ArgCount == 1) return 0;
	if (!ArgCount) lua_pushnumber(l, Math::RandomFloat());
	else lua_pushnumber(l, Math::RandomFloat((float)lua_tonumber(l, 1), (float)lua_tonumber(l, 2)));
	return 1;
}
//---------------------------------------------------------------------

bool RegisterGlobals()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_register(l, "print", Print);
	lua_register(l, "RandomInt", RandomInt);
	lua_register(l, "RandomFloat", RandomFloat);

	OK;
}
//---------------------------------------------------------------------

}