#include <Scripting/ScriptServer.h>
#include <AI/AIServer.h>
#include <Game/GameServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for the navigation mesh and regions

namespace SI
{

// Locks named region on the navmesh (by agent raduis or on all meshes), making it impassable
int NavMesh_LockRegion(lua_State* l)
{
	// Args: Level ID, Region ID (name), [target actor raduis]

	int ArgCount = lua_gettop(l);

	if (ArgCount < 2 || !lua_isstring(l, 1) || !lua_isstring(l, 2))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	float Radius = (ArgCount > 2) ? (float)lua_tonumber(l, 3) : 0.f;
	Game::CGameLevel* pLevel = GameSrv->GetLevel(CStrID(lua_tostring(l, 1)));
	if (pLevel && pLevel->GetAI()) pLevel->GetAI()->SetNavRegionFlags(CStrID(lua_tostring(l, 2)), NAV_FLAG_LOCKED, Radius);

	return 0;
}
//---------------------------------------------------------------------

// Unlocks named region on the navmesh (by agent raduis or on all meshes), making it passable
int NavMesh_UnlockRegion(lua_State* l)
{
	// Args: Level ID, Region ID (name), [target actor raduis]

	int ArgCount = lua_gettop(l);

	if (ArgCount < 2 || !lua_isstring(l, 1) || !lua_isstring(l, 2))
	{
		lua_settop(l, 0); //???need to call manually?
		return 0;
	}

	float Radius = (ArgCount > 2) ? (float)lua_tonumber(l, 3) : 0.f;
	Game::CGameLevel* pLevel = GameSrv->GetLevel(CStrID(lua_tostring(l, 1)));
	if (pLevel && pLevel->GetAI()) pLevel->GetAI()->ClearNavRegionFlags(CStrID(lua_tostring(l, 2)), NAV_FLAG_LOCKED, Radius);

	return 0;
}
//---------------------------------------------------------------------

bool RegisterNavMesh()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 2);
	ScriptSrv->ExportCFunction("LockRegion", NavMesh_LockRegion);
	ScriptSrv->ExportCFunction("UnlockRegion", NavMesh_UnlockRegion);
	lua_setglobal(l, "NavMesh");

	OK;
}
//---------------------------------------------------------------------

}