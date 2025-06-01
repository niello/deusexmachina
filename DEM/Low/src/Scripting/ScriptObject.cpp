#include "ScriptObject.h"

#include "ScriptServer.h"
#include <Events/EventServer.h>
#include <Data/Buffer.h>
#include <IO/IOServer.h>
#include <IO/Stream.h>
#include <Core/Factory.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Scripting
{
FACTORY_CLASS_IMPL(Scripting::CScriptObject, 'SOBJ', DEM::Core::CObject);

CScriptObject::~CScriptObject()
{
	ScriptSrv->RemoveObject(Name.CStr(), Table.CStr());
}
//---------------------------------------------------------------------

bool CScriptObject::Init(const char* LuaClassName)
{
	return ScriptSrv->CreateObject(*this, LuaClassName);
}
//---------------------------------------------------------------------

CScriptObject* CScriptObject::GetFromStack(lua_State* l, int StackIdx)
{
	if (!lua_istable(l, StackIdx))
	{
		Sys::Message("Can't get 'this' table, may be you used '.' instead of ':' for member call\n");
		lua_settop(l, 0);
		return nullptr;
	}

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, StackIdx);
	return (CScriptObject*)lua_touserdata(l, -1);
}
//---------------------------------------------------------------------

int CScriptObject_Index(lua_State* l)
{
	// Stack: current table at 1, key at 2

	// If assertion failed, must process non-string keys
	n_assert(lua_type(l, 2) == LUA_TSTRING);

	const char* Key = lua_tostring(l, 2);

	//???can return 'this' through __index?
	//!!!use userdata objects as class instances, if possible! cpp_ptr is rewritable
	//through Lua since index/newindex aren't called for existing fields
	n_assert_dbg(strcmp(Key, "cpp_ptr"));

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			lua_pushstring(l, This->GetName().CStr());
			return 1;
		}

		int Pushed = This->GetField(Key);

		if (Pushed > 0) return Pushed;
	}

	lua_getmetatable(l, 1);
	lua_getfield(l, -1, Key);
	return 1;
}
//---------------------------------------------------------------------

int CScriptObject_NewIndex(lua_State* l)
{
	// Stack: current table at 1, key at 2, value at 3

	// If assertion failed, must process non-string keys
	n_assert(lua_type(l, 2) == LUA_TSTRING);

	const char* Key = lua_tostring(l, 2);

	n_assert_dbg(strcmp(Key, "cpp_ptr"));

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			n_assert(lua_type(l, 3) == LUA_TSTRING);
			This->SetName(lua_tostring(l, 3));

			// Never cache the name, cause next rewrite will not call __newindex and will break object naming
			// it's not fatal if object can someway store its table instead of getting it from globals by name
			return 0;
		}

		Data::CData Data;
		if (ScriptSrv->LuaStackToData(Data, 3) && This->SetField(Key, Data)) return 0;
	}

	lua_pushvalue(l, 2);
	lua_pushvalue(l, 3);
	lua_rawset(l, 1);

	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_SubscribeEvent(lua_State* l)
{
	// Args: ScriptObject's this table, event name, [func name = event name]

	//!!!PRIORITY!
	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->SubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_UnsubscribeEvent(lua_State* l)
{
	//args: ScriptObject's this table, event name, [func name = event name]

	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->UnsubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

UPTR CScriptObject::LoadScriptFile(const char* pFileName)
{
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) FAIL;
	auto Buffer = File->ReadAll();
	if (!Buffer) FAIL;

	return LoadScript((const char*)Buffer->GetConstPtr(), Buffer->GetSize());
}
//---------------------------------------------------------------------

UPTR CScriptObject::LoadScript(const char* Buffer, UPTR Length)
{
	lua_State* l = ScriptSrv->GetLuaState();

	if (luaL_loadbuffer(l, Buffer, Length, Buffer) != 0)
	{
		Sys::Log("Error parsing script for %s: %s\n", Name.CStr(), lua_tostring(l, -1));
		Sys::Log("Script is: %s\n", Buffer);
		lua_pop(l, 1);
		return Error_Scripting_Parsing;
	}

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr()))
	{
		Sys::Log("Error: script object \"%s.%s\" not found\n",
			Name.CStr(), Table.IsEmpty() ? "_G" : Table.CStr());
		lua_pop(l, 1);
		return Error_Scripting_NoObject;
	}

	//if (Table.IsValid()) lua_remove(l, -2);
	//lua_setfenv(l, -2);

	//UPTR Result = RunFunctionInternal("<LOADING NEW SCRIPT>", 0, nullptr);
	//if (ExecResultIsError(Result)) Sys::Log("Script is: %s\n", Buffer);
	//return Result;
	return Error_Scripting_NoObject;
}
//---------------------------------------------------------------------

UPTR CScriptObject::PrepareToLuaCall(const char* pFuncName) const
{
	n_assert(pFuncName);

	lua_State* l = ScriptSrv->GetLuaState();

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr())) return Error_Scripting_NoObject;

	if (Table.IsValid()) lua_remove(l, -2);

	lua_getfield(l, -1, pFuncName); //???rawget?
	if (!lua_isfunction(l, -1)) 
	{
		lua_pop(l, 2);
		return Error_Scripting_NoFunction;
	}

	// Set env for the case when function is inherited from metatable
	//lua_pushvalue(l, -2);
	//lua_setfenv(l, -2);

	return Success;
}
//---------------------------------------------------------------------

UPTR CScriptObject::RunFunctionInternal(const char* pFuncName, int ArgCount, Data::CData* pRetVal) const
{
	UPTR Result = ScriptSrv->PerformCall(ArgCount, pRetVal, (Name + "." + pFuncName).CStr());
	if (ExecResultIsError(Result)) lua_pop(ScriptSrv->GetLuaState(), 1); // Object itself
	return Result;
}
//---------------------------------------------------------------------

UPTR CScriptObject::RunFunction(const char* pFuncName, const char* ArgLuaGlobal, Data::CData* pRetVal) const
{
	UPTR Res = PrepareToLuaCall(pFuncName);
	if (ExecResultIsError(Res)) return Res;
	lua_getglobal(ScriptSrv->GetLuaState(), ArgLuaGlobal); //???only globals are allowed? //???assert nil?
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

UPTR CScriptObject::RunFunction(const char* pFuncName, Data::CData* Args, UPTR ArgCount, Data::CData* pRetVal) const
{
	UPTR Res = PrepareToLuaCall(pFuncName);
	if (ExecResultIsError(Res)) return Res;
	for (UPTR i = 0; i < ArgCount; ++i)
		ScriptSrv->DataToLuaStack(Args[i]);
	return RunFunctionInternal(pFuncName, ArgCount, pRetVal);
}
//---------------------------------------------------------------------

UPTR CScriptObject::RunFunctionOneArg(const char* pFuncName, const Data::CData& Arg, Data::CData* pRetVal) const
{
	UPTR Res = PrepareToLuaCall(pFuncName);
	if (ExecResultIsError(Res)) return Res;
	ScriptSrv->DataToLuaStack(Arg);
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, const char* HandlerFuncName, Events::CEventDispatcher* pDisp, U16 Priority)
{
	NOT_IMPLEMENTED_MSG("CScriptObject is deprecated and will be removed");
	//auto Sub = pDisp->Subscribe(EventID, n_new(Events::CEventHandlerScript)(this, HandlerFuncName, Priority));
	//if (!Sub) FAIL;
	//Subscriptions.Add(std::move(Sub));
	OK;
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, const char* HandlerFuncName, const Events::CEventDispatcher* pDisp)
{
	NOT_IMPLEMENTED_MSG("CScriptObject is deprecated and will be removed");
	/*
	for (UPTR i = 0; i < Subscriptions.GetCount(); ++i)
	{
		Events::PSub CurrSub = Subscriptions[i];
		if (CurrSub->GetEvent() == EventID && CurrSub->GetDispatcher() == pDisp &&
			((Events::CEventHandlerScript*)CurrSub->GetHandler())->GetFunc() == HandlerFuncName)
		{
			Subscriptions.RemoveAt(i);
			break; //???or scan all array for duplicates?
		}
	}
	*/
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, const char* HandlerFuncName, U16 Priority)
{
	return SubscribeEvent(EventID, HandlerFuncName, EventSrv, Priority);
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, const char* HandlerFuncName)
{
	UnsubscribeEvent(EventID, HandlerFuncName, EventSrv);
}
//---------------------------------------------------------------------

void CScriptObject::SetName(const char* NewName)
{
	n_assert(NewName && *NewName);

	if (Name == NewName) return;

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr())) return;

	//int TableIdx = Table.IsValid() ? -2 : LUA_GLOBALSINDEX;
	//lua_State* l = ScriptSrv->GetLuaState();
	//lua_setfield(l, TableIdx, NewName);
	//lua_pushnil(l);
	//lua_setfield(l, TableIdx, Name.CStr());
	Name = NewName;
}
//---------------------------------------------------------------------

}
