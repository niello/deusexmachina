#include "EntityScriptObject.h"

#include <Game/Entity.h>
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

using namespace Data;

int CEntityScriptObject_SubscribeLocalEvent(lua_State* l)
{
	//args: EntityScriptObject's this table, event name, [func name = event name]
	SETUP_ENT_SI_ARGS(2)

	//!!!PRIORITY!
	if (This) This->SubscribeLocalEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

int CEntityScriptObject_UnsubscribeLocalEvent(lua_State* l)
{
	//args: EntityScriptObject's this table or nil, event name, [func name = event name]
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

	PParams Params;
	if (ArgCount > 2 && !lua_isnil(l, 3))
	{
		CData Data;
		ScriptSrv->LuaStackToData(Data, 3, l);
		if (lua_istable(l, 3)) Params = (PParams)Data;
		else if (Data.IsValid())
		{
			//???for all args read and push into array?
			Params = n_new(CParams(1));
			Params->Set(CStrID::Empty, Data);
		}
	}

	This->GetEntity()->FireEvent(CStrID(lua_tostring(l, 2)), Params);
	return 0;
}
//---------------------------------------------------------------------

bool CEntityScriptObject::RegisterClass()
{
	if (ScriptSrv->BeginClass("CEntityScriptObject"))
	{
		//???use virtual GetField, SetField or override index & newindex?
		ScriptSrv->ExportCFunction("__index", CScriptObject_Index);
		ScriptSrv->ExportCFunction("__newindex", CScriptObject_NewIndex);
		ScriptSrv->ExportCFunction("SubscribeLocalEvent", CEntityScriptObject_SubscribeLocalEvent);
		ScriptSrv->ExportCFunction("UnsubscribeLocalEvent", CEntityScriptObject_UnsubscribeLocalEvent);
		ScriptSrv->ExportCFunction("FireEvent", CEntityScriptObject_FireEvent);
		ScriptSrv->EndClass();
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

int CEntityScriptObject::GetField(LPCSTR Key) const
{
	CData Data;
	if (GetEntity()->GetAttr(Data, CStrID(Key)))
	{
		int Pushed = ScriptSrv->DataToLuaStack(Data);
		if (Pushed > 0) return Pushed;
	}

	return CScriptObject::GetField(Key);
}
//---------------------------------------------------------------------

bool CEntityScriptObject::SetField(LPCSTR Key, const CData& Value)
{
	if (!strcmp(Key, "Transform")) FAIL; // Read-only, send SetTransform event to change

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