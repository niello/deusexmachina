#include <Scripting/ScriptServer.h>
#include <Scripting/ScriptObject.h>
#include <Game/Mgr/EntityManager.h>
//#include <Scripting/EventHandlerScript.h>
//#include <Events/EventManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script interface extension for CScriptObject Lua class

//int CScriptObject_SubscribeEntityEvent(lua_State* l);
//int CScriptObject_UnsubscribeEntityEvent(lua_State* l);

namespace SI
{
using namespace Scripting;
using namespace Events;

int CScriptObject_SubscribeEntityEvent(lua_State* l)
{
	//args: ScriptObject's this table, entity id, event name, [func name = event name]

	//!!!PRIORITY!
	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This)
	{
		Game::CEntity* pEntity = EntityMgr->GetEntityByID(CStrID(lua_tostring(l, 2)));
		if (pEntity) This->SubscribeEvent(CStrID(lua_tostring(l, 3)), lua_tostring(l, -2), pEntity, Priority_Default);
	}
	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_UnsubscribeEntityEvent(lua_State* l)
{
	//args: EntityScriptObject's this table or nil, entity id, event name, [func name = event name]

	//???can dispacther die before it's clients unsubscribe?
	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This)
	{
		Game::CEntity* pEntity = EntityMgr->GetEntityByID(CStrID(lua_tostring(l, 2)));
		if (pEntity) This->UnsubscribeEvent(CStrID(lua_tostring(l, 3)), lua_tostring(l, -2), pEntity);
	}
	return 0;
}
//---------------------------------------------------------------------

bool RegisterScriptObjectSIEx()
{
	if (ScriptSrv->BeginExistingClass("CScriptObject"))
	{
		ScriptSrv->ExportCFunction("SubscribeEntityEvent", CScriptObject_SubscribeEntityEvent);
		ScriptSrv->ExportCFunction("UnsubscribeEntityEvent", CScriptObject_UnsubscribeEntityEvent);
		ScriptSrv->EndClass();
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

}