#include "ScriptObject.h"

#include "ScriptServer.h"
#include "EventHandlerScript.h"
#include <Events/EventManager.h>
#include <Data/DataServer.h>
#include <DB/Dataset.h>
#include <DB/Database.h>
#include <DB/DBServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Attr
{
	DefineString(LuaObjName);
	DefineString(LuaFieldName);
	DefineAttr(LuaValue);
}

BEGIN_ATTRS_REGISTRATION(ScriptObject)
	RegisterString(LuaObjName, ReadWrite);
	RegisterString(LuaFieldName, ReadWrite);
	RegisterVarAttr(LuaValue, ReadWrite);
END_ATTRS_REGISTRATION

extern const nString StrLuaObjects("LuaObjects");

namespace Scripting
{
ImplementRTTI(Scripting::CScriptObject, Core::CRefCounted);
ImplementFactory(Scripting::CScriptObject);

CScriptObject::~CScriptObject()
{
	//???if (Temporary && DBSaveLoadEnabled) ClearFields();? Temporary is Lua-exposed flag too.
	ScriptSrv->RemoveObject(Name.Get(), Table.Get());
}
//---------------------------------------------------------------------

bool CScriptObject::Init(LPCSTR LuaClassName)
{
	return ScriptSrv->CreateObject(*this, LuaClassName);
}
//---------------------------------------------------------------------

CScriptObject* CScriptObject::GetFromStack(lua_State* l, int StackIdx)
{
	if (!lua_istable(l, StackIdx))
	{
		n_message("Can't get 'this' table, may be you used '.' instead of ':' for member call\n");
		lua_settop(l, 0);
		return NULL;
	}

	//???why don't work: lua_getfield(l, LUA_ENVIRONINDEX, "cpp_ptr");
	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, StackIdx);
	return (CScriptObject*)lua_touserdata(l, -1);
}
//---------------------------------------------------------------------

// Special cases cpp_ptr & name, if not, tries to get value from C++ class,
// else reads value from class table (metatable of current)
int CScriptObject_Index(lua_State* l)
{
	// stack contains: current table at 1, key at 2

	LPCSTR Key = lua_tostring(l, 2);

	//never called O_o
	//if (!strcmp(Key, "cpp_ptr"))
	//{
	//	//lua_pushnil(l);
	//	return 0;
	//}

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			lua_pushstring(l, This->GetName().Get());
			return 1;
		}

		if (!strcmp(Key, "SaveLoadEnabled"))
		{
			lua_pushboolean(l, This->DBSaveLoadEnabled);
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

// Special cases cpp_ptr & name, if not, tries to set value to C++ class,
// else writes value to class table (metatable of current)
int CScriptObject_NewIndex(lua_State* l)
{
	// stack contains: current table at 1, key at 2, value at 3

	const char* Key = lua_tostring(l, 2);

	//???never called? //???the same for "this"?
	if (!strcmp(Key, "cpp_ptr")) return 0; // disallow to rewrite this data

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			n_assert(lua_isstring(l, 3));
			This->SetName(lua_tostring(l, 3));

			// never cache the name, cause next rewrite will not call __newindex and will break object naming
			// it's not fatal if object can someway store its table instead of getting it from globals by name
			return 0;
		}

		if (!strcmp(Key, "SaveLoadEnabled"))
		{
			This->DBSaveLoadEnabled = (lua_toboolean(l, 3) != 0);
			return 0;
		}

		CData Data;
		if (ScriptSrv->LuaStackToData(Data, 3, l) && This->SetField(Key, Data)) return 0;
	}

	lua_pushvalue(l, 2);
	lua_pushvalue(l, 3);
	lua_rawset(l, 1);

	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_SubscribeEvent(lua_State* l)
{
	//args: ScriptObject's this table, event name, [func name = event name]

	//!!!PRIORITY!
	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->SubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_UnsubscribeEvent(lua_State* l)
{
	//args: ScriptObject's this table or nil, event name, [func name = event name]

	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->UnsubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::LoadScriptFile(const nString& FileName)
{
	CBuffer Buffer;
	if (!DataSrv->LoadFileToBuffer(FileName, Buffer) &&
		!DataSrv->LoadFileToBuffer("scripts:" + FileName + ".lua", Buffer)) return Error;
	return LoadScript((LPCSTR)Buffer.GetPtr(), Buffer.GetSize());
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::LoadScript(LPCSTR Buffer, DWORD Length)
{
	lua_State* l = ScriptSrv->GetLuaState();

	if (luaL_loadbuffer(l, Buffer, Length, Buffer) != 0)
	{
		n_printf("Error parsing script for %s: %s\n", Name.Get(), lua_tostring(l, -1));
		n_printf("Script is: %s\n", Buffer);
		lua_pop(l, 1);
		return Error;
	}

	if (!ScriptSrv->PlaceObjectOnStack(Name.Get(), Table.Get()))
	{
		lua_pop(l, 1);
		return Error;
	}

	if (Table.IsValid()) lua_remove(l, -2);
	lua_setfenv(l, -2);

	EExecStatus Result = RunFunctionInternal("<LOADING NEW SCRIPT>", 0, NULL);
	if (Result == Error) n_printf("Script is: %s\n", Buffer);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptObject::PrepareToLuaCall(LPCSTR pFuncName) const
{
	n_assert(pFuncName);

	lua_State* l = ScriptSrv->GetLuaState();

	if (!ScriptSrv->PlaceObjectOnStack(Name.Get(), Table.Get())) FAIL;

	if (Table.IsValid()) lua_remove(l, -2);

	lua_getfield(l, -1, pFuncName);
	if (!lua_isfunction(l, -1)) 
	{
		n_printf("Error: function \"%s\" not found in script object \"%s\"\n", pFuncName, Name.Get());
		lua_pop(l, 2);
		FAIL;
	}

	// Set env for the case when function is inherited from metatable
	lua_pushvalue(l, -2);
	lua_setfenv(l, -2);

	OK;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunctionInternal(LPCSTR pFuncName, int ArgCount, CData* pRetVal) const
{
	EExecStatus Result = ScriptSrv->PerformCall(ArgCount, pRetVal, (Name + "." + pFuncName).Get());
	if (Result == Error) lua_pop(ScriptSrv->GetLuaState(), 1); // Object itself
	return Result;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunction(LPCSTR pFuncName, LPCSTR LuaArg, CData* pRetVal) const
{
	if (!PrepareToLuaCall(pFuncName)) return Error;
	lua_getglobal(ScriptSrv->GetLuaState(), LuaArg); //???only globals are allowed? //???assert nil?
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunctionData(LPCSTR pFuncName, const CData& Arg, CData* pRetVal) const
{
	if (!PrepareToLuaCall(pFuncName)) return Error;
	ScriptSrv->DataToLuaStack(Arg);
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, CEventDispatcher* pDisp, ushort Priority)
{
	PSub Sub = pDisp->AddHandler(EventID, n_new(CEventHandlerScript)(this, HandlerFuncName, Priority));
	if (!Sub.isvalid()) FAIL;
	Subscriptions.Append(Sub);
	OK;
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, const CEventDispatcher* pDisp)
{
	for (int i = 0; i < Subscriptions.Size(); i++)
	{
		PSub CurrSub = Subscriptions[i];
		if (CurrSub->GetEvent() == EventID && CurrSub->GetDispatcher() == pDisp &&
			((CEventHandlerScript*)CurrSub->GetHandler())->GetFunc() == HandlerFuncName)
		{
			Subscriptions.Erase(i);
			break; //???or scan all array for duplicates?
		}
	}
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority)
{
	return SubscribeEvent(EventID, HandlerFuncName, EventMgr, Priority);
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName)
{
	UnsubscribeEvent(EventID, HandlerFuncName, EventMgr);
}
//---------------------------------------------------------------------

void CScriptObject::SetName(const char* NewName)
{
	n_assert(NewName && *NewName);

	if (Name == NewName) return;

	if (!ScriptSrv->PlaceObjectOnStack(Name.Get(), Table.Get())) return;

	int TableIdx = Table.IsValid() ? -2 : LUA_GLOBALSINDEX;
	lua_State* l = ScriptSrv->GetLuaState();
	lua_setfield(l, TableIdx, NewName);
	lua_pushnil(l);
	lua_setfield(l, TableIdx, Name.Get());
	Name = NewName;
}
//---------------------------------------------------------------------

bool CScriptObject::SaveFields(DB::CDatabase* pDB)
{
	n_assert(pDB);

	if (!DBSaveLoadEnabled) OK;

	//RunFunction("OnSave");
	//!!!there is object on stack now, so we need not to call PlaceObjectOnStack!

	if (!ScriptSrv->PlaceObjectOnStack(Name.Get(), Table.Get())) FAIL;

	nString ObjName = GetFullName();

	//!!!optimize - query can be compiled once! (for both save & load, or load can avoid reading objname!)
	//!!!can lookup table once per db loading! (inside a script server)
	//!!!reuse VT (& DS?) without reallocation!
	DB::PDataset DS = pDB->GetTable(StrLuaObjects)->CreateDataset();
	//DS->AddColumnsFromTable(); // LuaObjName LuaFieldName LuaValue
	DS->SetWhereClause("LuaObjName='" + ObjName + "'");
	DS->PerformQuery();

	DB::PValueTable VT = DS->GetValueTable();
	int InitialCount = VT->GetRowCount();
	int Written = 0;

	lua_State* l = ScriptSrv->GetLuaState();

	lua_pushnil(l);
	while (lua_next(l, -2))
	{
		nString Key = lua_tostring(l, -2);
		if (!lua_isfunction(l, -1) &&
			Key != "name" && Key != "this" && Key != "cpp_ptr" && Key != "SaveLoadEnabled")
		{
			int RowIdx = INVALID_INDEX;
			for (int i = 0; i < InitialCount; i++)
				if (VT->IsRowUntouched(i) && VT->Get<nString>(1, 0) == Key)
				{
					RowIdx = i;
					//???check values equality & continue outer loop if equal?
					break;
				}
			
			if (RowIdx == INVALID_INDEX)
			{
				RowIdx = VT->AddRow();
				VT->Set(0, RowIdx, ObjName);
				VT->Set(1, RowIdx, Key);
			}
			
			CData Val;
			ScriptSrv->LuaStackToData(Val, -1, l);
			VT->SetValue(2, RowIdx, Val);
			
			++Written;
		}
		lua_pop(l, 1);
	}

	lua_pop(l, Table.IsValid() ? 2 : 1);

	// Never need to save metatable fields, all readwrite values are stored in this table
	// Metatable contains only readonly defaults

	//???or delete all rows at the beginning and then insert all fields as new? What is better?
	for (int i = 0; i < InitialCount; i++)
		if (VT->IsRowUntouched(i)) VT->DeleteRow(i);

	if (InitialCount || Written) DS->CommitChanges();

	OK;
}
//---------------------------------------------------------------------

bool CScriptObject::LoadFields(const DB::CDatabase* pDB)
{
	n_assert(pDB);

	if (!DBSaveLoadEnabled) OK;
	
	int Idx = pDB->FindTableIndex(StrLuaObjects);
	if (Idx == INVALID_INDEX) OK;

	lua_State* l = ScriptSrv->GetLuaState();

	if (!ScriptSrv->PlaceObjectOnStack(Name.Get(), Table.Get())) FAIL;

	//!!!no need in reading objname!

	nString ObjName = GetFullName();
	static const DB::CAttrID Columns[2] = { Attr::LuaFieldName, Attr::LuaValue };

	DB::PDataset DS = pDB->GetTable(Idx)->CreateDataset();
	DS->AddColumns(Columns, 2);
	DS->SetWhereClause("LuaObjName='" + ObjName + "'");
	DS->PerformQuery();

	CData Value;
	for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
	{
		lua_pushstring(l, DS->GetValueTable()->Get<nString>(0, i).Get());
		DS->GetValueTable()->GetValue(1, i, Value);
		if (ScriptSrv->DataToLuaStack(Value) == 1) lua_rawset(l, -3);
	}

	lua_pop(l, Table.IsValid() ? 2 : 1);

	RunFunction("OnLoad"); //???call before pop without PlaceOnStack?

	OK;
}
//---------------------------------------------------------------------

void CScriptObject::ClearFields(DB::CDatabase* pDB)
{
	if (!DBSaveLoadEnabled) return;
	ClearFieldsDeffered(pDB, GetFullName());
}
//---------------------------------------------------------------------

void CScriptObject::ClearFieldsDeffered(DB::CDatabase* pDB, const nString FullObjName)
{
	//!!!hastable lookup can be avoided with shared dataset existence check!
	if (!pDB->HasTable(StrLuaObjects)) return;
	nString SQL("DELETE FROM LuaObjects WHERE LuaObjName=\"");
	DB::PCommand Cmd = DB::CCommand::Create();
	n_assert(Cmd->Execute(pDB, SQL + FullObjName + "\""));
}
//---------------------------------------------------------------------

} //namespace AI
