#include <Scripting/ScriptServer.h>
#include <Game/GameServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for the game server

namespace SI
{

int CGameServer_SetGlobal(lua_State* l)
{
	// Args: Name, Value

	int ArgCount = lua_gettop(l);

	if (ArgCount < 2 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	Data::CData Value;
	ScriptSrv->LuaStackToData(Value, 2);
	GameSrv->SetGlobalAttr(CStrID(lua_tostring(l, 1)), Value);
	return 0;
}
//---------------------------------------------------------------------

int CGameServer_GetGlobal(lua_State* l)
{
	// Args: Name

	int ArgCount = lua_gettop(l);

	if (ArgCount < 1 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	Data::CData Value;
	if (!GameSrv->GetGlobalAttr(Value, CStrID(lua_tostring(l, 1)))) return 0;
	return ScriptSrv->DataToLuaStack(Value);
}
//---------------------------------------------------------------------

int CGameServer_HasGlobal(lua_State* l)
{
	// Args: Name

	int ArgCount = lua_gettop(l);

	if (ArgCount < 1 || !lua_isstring(l, 1))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	lua_pushboolean(l, GameSrv->HasGlobalAttr(CStrID(lua_tostring(l, 1))));
	return 1;
}
//---------------------------------------------------------------------

bool RegisterGameServer()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 2);
	ScriptSrv->ExportCFunction("SetGlobal", CGameServer_SetGlobal);
	ScriptSrv->ExportCFunction("GetGlobal", CGameServer_GetGlobal);
	ScriptSrv->ExportCFunction("HasGlobal", CGameServer_HasGlobal);
	lua_setglobal(l, "GameSrv");

	OK;
}
//---------------------------------------------------------------------

}