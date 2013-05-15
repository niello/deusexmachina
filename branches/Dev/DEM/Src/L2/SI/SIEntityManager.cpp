#include <Scripting/ScriptServer.h>
#include <Game/EntityManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for the entity manager

namespace SI
{

int CEntityManager_RemoveEntity(lua_State* l)
{
	// args: Entity ID or alias
	if (lua_gettop(l) > 0) EntityMgr->RemoveEntity(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

int CEntityManager_DeleteEntity(lua_State* l)
{
	// args: Entity ID or alias
	if (lua_gettop(l) > 0) EntityMgr->DeleteEntity(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

bool RegisterEntityManager()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 2);
	ScriptSrv->ExportCFunction("RemoveEntity", CEntityManager_RemoveEntity);
	ScriptSrv->ExportCFunction("DeleteEntity", CEntityManager_DeleteEntity);
	lua_setglobal(l, "EntityMgr");

	OK;
}
//---------------------------------------------------------------------

}