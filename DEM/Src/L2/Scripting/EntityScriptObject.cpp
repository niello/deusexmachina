#include "EntityScriptObject.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Game/EntityManager.h>
#include <Events/Subscription.h>
#include <Scripting/ScriptServer.h>
#include <Data/Params.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Scripting
{
__ImplementClass(Scripting::CEntityScriptObject, 'ESCO', CScriptObject);

int CEntityScriptObject_SubscribeLocalEvent(lua_State* l)
{
	// Args: EntityScriptObject's this table, event name, [func name = event name, int priority = default]
	SETUP_ENT_SI_ARGS(2)

	if (This)
	{
		LPCSTR pHandlerName = (ArgCount < 4) ? lua_tostring(l, -2) : lua_tostring(l, 3);
		ushort Priority = (ArgCount < 4) ? Events::Priority_Default : (ushort)lua_tointeger(l, 4);
		This->SubscribeLocalEvent(CStrID(lua_tostring(l, 2)), pHandlerName, Priority);
	}
	return 0;
}
//---------------------------------------------------------------------

int CEntityScriptObject_UnsubscribeLocalEvent(lua_State* l)
{
	// Args: EntityScriptObject's this table or nil, event name, [func name = event name]
	SETUP_ENT_SI_ARGS(2)

	if (This) This->UnsubscribeLocalEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

int CEntityScriptObject_FireEvent(lua_State* l)
{
	//args: EntityScriptObject's this table or nil, event name, [parameter or parameter table]
	SETUP_ENT_SI_ARGS(3)

	if (!lua_isstring(l, 2))
	{
		lua_settop(l, 0);
		return 0;
	}

	Data::PParams Params;
	if (ArgCount > 2 && !lua_isnil(l, 3))
	{
		Data::CData Data;
		ScriptSrv->LuaStackToData(Data, 3, l);
		if (lua_istable(l, 3)) Params = (Data::PParams)Data;
		else if (Data.IsValid())
		{
			//???for all args read and push into array?
			Params = n_new(Data::CParams(1));
			Params->Set(CStrID::Empty, Data);
		}
	}

	This->GetEntity()->FireEvent(CStrID(lua_tostring(l, 2)), Params);
	return 0;
}
//---------------------------------------------------------------------

int CEntityScriptObject_AttachProperty(lua_State* l)
{
	// Args: EntityScriptObject's this table, property class name
	SETUP_ENT_SI_ARGS(2)
	if (lua_isstring(l, 2))
	{
		Game::CProperty* pProp = EntityMgr->AttachProperty(*This->GetEntity(), CString(lua_tostring(l, 2)));
		pProp->Activate();
	}
	return 0;
}
//---------------------------------------------------------------------

int CEntityScriptObject_RemoveProperty(lua_State* l)
{
	// Args: EntityScriptObject's this table, property class name
	SETUP_ENT_SI_ARGS(2)
	if (lua_isstring(l, 2)) EntityMgr->RemoveProperty(*This->GetEntity(), CString(lua_tostring(l, 2)));
	return 0;
}
//---------------------------------------------------------------------

bool CEntityScriptObject::RegisterClass()
{
	if (ScriptSrv->BeginClass("CEntityScriptObject", "CScriptObject"))
	{
		ScriptSrv->ExportCFunction("SubscribeLocalEvent", CEntityScriptObject_SubscribeLocalEvent);
		ScriptSrv->ExportCFunction("UnsubscribeLocalEvent", CEntityScriptObject_UnsubscribeLocalEvent);
		ScriptSrv->ExportCFunction("FireEvent", CEntityScriptObject_FireEvent);
		ScriptSrv->ExportCFunction("AttachProperty", CEntityScriptObject_AttachProperty);
		ScriptSrv->ExportCFunction("RemoveProperty", CEntityScriptObject_RemoveProperty);
		ScriptSrv->EndClass(true);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

int CEntityScriptObject::GetField(LPCSTR Key) const
{
	if (!strcmp(Key, "LevelID"))
	{
		lua_pushstring(ScriptSrv->GetLuaState(), pEntity->GetLevel()->GetID().CStr());
		return 1;
	}

	Data::CData Data;
	if (pEntity->GetAttr(Data, CStrID(Key)))
	{
		int Pushed = ScriptSrv->DataToLuaStack(Data);
		if (Pushed > 0) return Pushed;
	}

	return CScriptObject::GetField(Key);
}
//---------------------------------------------------------------------

bool CEntityScriptObject::SetField(LPCSTR Key, const Data::CData& Value)
{
	// Read-only
	if (!strcmp(Key, "Transform") || !strcmp(Key, "LevelID")) FAIL;

	GetEntity()->SetAttr(CStrID(Key), Value);
	OK;
}
//---------------------------------------------------------------------

bool CEntityScriptObject::SubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority)
{
	return SubscribeEvent(EventID, HandlerFuncName, pEntity, Priority);
}
//---------------------------------------------------------------------

void CEntityScriptObject::UnsubscribeLocalEvent(CStrID EventID, LPCSTR HandlerFuncName)
{
	UnsubscribeEvent(EventID, HandlerFuncName, pEntity);
}
//---------------------------------------------------------------------

}