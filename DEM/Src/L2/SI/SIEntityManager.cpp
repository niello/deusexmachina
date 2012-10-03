#include <Scripting/ScriptServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Game/Mgr/EntityManager.h>

// Script Interface for the Dialogue system

namespace SI
{

int CEntityManager_RemoveEntity(lua_State* l)
{
	//args: Entity ID or alias
	if (lua_gettop(l) > 0) EntityMgr->RemoveEntity(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

int CEntityManager_DeleteEntity(lua_State* l)
{
	//args: Entity ID or alias
	if (lua_gettop(l) > 0) EntityMgr->DeleteEntity(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

bool RegisterEntityManager()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 1);
	ScriptSrv->ExportCFunction("RemoveEntity", CEntityManager_RemoveEntity);
	ScriptSrv->ExportCFunction("DeleteEntity", CEntityManager_DeleteEntity);
	lua_setglobal(l, "EntityMgr");

	OK;
}
//---------------------------------------------------------------------

}