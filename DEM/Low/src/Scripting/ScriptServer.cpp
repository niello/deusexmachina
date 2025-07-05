#include "ScriptServer.h"

#include "ScriptObject.h"
#include <Events/EventServer.h>
#include <Data/DataArray.h>
#include <Data/ParamsUtils.h>
#include <Data/Buffer.h>
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>
#include <IO/IOServer.h>
#include <IO/Stream.h>

extern const CString StrLuaObjects;

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#define INITIAL_OBJ_TABLE_SIZE	8
#define TBL_CLASSES				"Classes"

namespace Scripting
{
__ImplementSingleton(CScriptServer);

CScriptServer::CScriptServer(): CurrClass(nullptr), CurrObj(nullptr)
{
	__ConstructSingleton;

	l = luaL_newstate();
	n_assert2(l, "Can't create main Lua interpreter");

	luaL_openlibs(l);

	// Create base class for scripted objects

	// 'Classes' table
	lua_createtable(l, 0, 16);

	// 'CScriptObject' table
	lua_createtable(l, 0, 4);

	// Create top-level index table to access global namespace
	lua_createtable(l, 0, 1);
	lua_getglobal(l, "_G");
	lua_setfield(l, -2, "__index");
	lua_setmetatable(l, -2);

	ExportCFunction("__index", CScriptObject_Index);
	ExportCFunction("__newindex", CScriptObject_NewIndex);
	ExportCFunction("SubscribeEvent", CScriptObject_SubscribeEvent);
	ExportCFunction("UnsubscribeEvent", CScriptObject_UnsubscribeEvent);
	lua_setfield(l, -2, "CScriptObject");

	lua_setglobal(l, TBL_CLASSES);
}
//---------------------------------------------------------------------

CScriptServer::~CScriptServer()
{
	if (l)
	{
		lua_close(l);
		l = nullptr;
	}

	__DestructSingleton;
}
//---------------------------------------------------------------------

int CScriptServer::DataToLuaStack(const Data::CData& Data)
{
	if (Data.IsVoid()) lua_pushnil(l);
	else if (Data.IsA<bool>()) lua_pushboolean(l, Data.GetValue<bool>());
	else if (Data.IsA<int>()) lua_pushinteger(l, Data.GetValue<int>());
	else if (Data.IsA<float>()) lua_pushnumber(l, Data.GetValue<float>());
	else if (Data.IsA<CString>())
	{
		const char* pStr = Data.GetValue<CString>().CStr();
		lua_pushstring(l, pStr ? pStr : "");
	}
	else if (Data.IsA<CStrID>())
	{
		const char* pStr = Data.GetValue<CStrID>().CStr();
		lua_pushstring(l, pStr ? pStr : "");
	}
	else if (Data.IsA<PVOID>()) lua_pushlightuserdata(l, Data.GetValue<PVOID>());
	else if (Data.IsA<vector3>())
	{
		const vector3& Vector = Data.GetValue<vector3>();
		lua_createtable(l, 0, 3);
		lua_pushstring(l, "x");
		lua_pushnumber(l, Vector.x);
		lua_rawset(l, -3);
		lua_pushstring(l, "y");
		lua_pushnumber(l, Vector.y);
		lua_rawset(l, -3);
		lua_pushstring(l, "z");
		lua_pushnumber(l, Vector.z);
		lua_rawset(l, -3);
	}
	else if (Data.IsA<vector4>())
	{
		const vector4& Vector = Data.GetValue<vector4>();
		lua_createtable(l, 0, 4);
		lua_pushstring(l, "x");
		lua_pushnumber(l, Vector.x);
		lua_rawset(l, -3);
		lua_pushstring(l, "y");
		lua_pushnumber(l, Vector.y);
		lua_rawset(l, -3);
		lua_pushstring(l, "z");
		lua_pushnumber(l, Vector.z);
		lua_rawset(l, -3);
		lua_pushstring(l, "w");
		lua_pushnumber(l, Vector.w);
		lua_rawset(l, -3);
	}
	else if (Data.IsA<Data::PDataArray>())
	{
		const Data::CDataArray& A = *Data.GetValue<Data::PDataArray>();
		lua_createtable(l, A.GetCount(), 0);
		for (UPTR i = 0; i < A.GetCount();)
			if (DataToLuaStack(A[i]) == 1)
				lua_rawseti(l, -2, ++i);
	}
	else if (Data.IsA<Data::PParams>())
	{
		const Data::CParams& P = *Data.GetValue<Data::PParams>();
		lua_createtable(l, 0, P.GetCount());
		for (UPTR i = 0; i < P.GetCount(); ++i)
		{
			lua_pushstring(l, P[i].GetName().CStr());
			if (DataToLuaStack(P[i].GetRawValue()) == 1) lua_rawset(l, -3);
			else lua_pop(l, 1);
		}
	}
	else
	{
		//???type string or value string?
		DBG_ONLY(Sys::Log("Can't push data to Lua stack: <%s>\n", Data.ToString()));
		return 0;
	}

	return 1;
}
//---------------------------------------------------------------------

bool CScriptServer::LuaStackToData(Data::CData& Result, int StackIdx)
{
	if (StackIdx < 0) StackIdx = lua_gettop(l) + StackIdx + 1;

	int Type = lua_type(l, StackIdx);
	switch (Type)
	{
		case LUA_TNONE:
		case LUA_TNIL:				Result.Clear(); OK;
		case LUA_TBOOLEAN:			Result = (lua_toboolean(l, StackIdx) != 0); OK;
		case LUA_TSTRING:			Result = CString(lua_tostring(l, StackIdx)); OK;
		case LUA_TLIGHTUSERDATA:	Result = lua_touserdata(l, StackIdx); OK;
		case LUA_TNUMBER:
		{
			//double Value = lua_tonumber(l, StackIdx);
			//int IntValue;
			//lua_number2int(IntValue, Value);
			//if (((double)IntValue) == Value) Result = IntValue;
			//else Result = (float)Value;
			OK;
		}
		case LUA_TTABLE:
		{						
			int Key, MaxKey = -1;	
			
			lua_pushnil(l);
			while (lua_next(l, StackIdx))
				if (lua_type(l, -2) == LUA_TSTRING)
				{
					if (MaxKey > -1)
						Sys::Log("CScriptServer::LuaStackToData, Warning: mixed table, int & string keys, "
								 "will convert only string ones\n");
					MaxKey = -1;
					lua_pop(l, 2);
					break;
				}
				else
				{
					Key = (int)lua_tointeger(l, -2);
					if (Key <= 0)
					{
						Sys::Log("CScriptServer::LuaStackToData: Incorrect array index (Idx < 0)\n");
						lua_pop(l, 2);
						FAIL;
					}
					if (Key > MaxKey) MaxKey = Key;
					lua_pop(l, 1);
				}

			if (MaxKey > -1)
			{
				Data::PDataArray Array = n_new(Data::CDataArray);
				Array->Reserve(MaxKey);

				lua_pushnil(l);
				while (lua_next(l, StackIdx))
				{
					Key = (int)lua_tointeger(l, -2) - 1;
					LuaStackToData(Array->At(Key), -1);
					lua_pop(l, 1);
				}
				
				Result = Array;
			}
			else
			{
				Data::CData ParamData;
				Data::PParams Params = n_new(Data::CParams);
				
				lua_pushnil(l);
				while (lua_next(l, StackIdx))
					if (lua_type(l, -2) == LUA_TSTRING)
					{
						const char* pStr = lua_tostring(l, -2);

						// Prevent diving into possible self-references
						if (strcmp(pStr, "__index") &&
							strcmp(pStr, "__newindex") &&
							strcmp(pStr, "this"))
						{
							LuaStackToData(ParamData, -1);
							Params->Set(CStrID(pStr), ParamData);
						}
						
						lua_pop(l, 1);
					}
				
				Result = Params;
			}
		}
		case LUA_TFUNCTION:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:		FAIL;
		default:
		{
			Sys::Log("Conversion from Lua to CData failed, unknown Lua type %d\n", Type);
			FAIL;
		}
	}
}
//---------------------------------------------------------------------

UPTR CScriptServer::RunScriptFile(const char* pFileName)
{
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) FAIL;
	auto Buffer = File->ReadAll();
	if (!Buffer) FAIL;

	return RunScript((const char*)Buffer->GetConstPtr(), Buffer->GetSize());
}
//---------------------------------------------------------------------

UPTR CScriptServer::RunScript(const char* Buffer, UPTR Length, Data::CData* pRetVal)
{
	UPTR Result;
	if (luaL_loadbuffer(l, Buffer, ((Length > -1) ? Length : strlen(Buffer)), Buffer) != 0)
	{
		Sys::Log("Error parsing script for ScriptSrv: %s\n", lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
		Result = Error_Scripting_Parsing;
	}
	else
	{
		Result = PerformCall(0, pRetVal, "script for ScriptSrv");
		if (!ExecResultIsError(Result)) return Result;
	}

	Sys::Log("Script is: %s\n", Buffer);
	return Result;
}
//---------------------------------------------------------------------

// Mainly for internal use
UPTR CScriptServer::PerformCall(int ArgCount, Data::CData* pRetVal, const char* pDbgName)
{
	int ResultCount = lua_gettop(l) - ArgCount - 1;

	if (lua_pcall(l, ArgCount, LUA_MULTRET, 0))
	{
		Sys::Log("Error running %s: %s\n", pDbgName, lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
		return Error;
	}

	ResultCount = lua_gettop(l) - ResultCount;
	//!!!coroutines can return Running!

	if (ResultCount < 1) return Success;

	if (pRetVal)
	{
		n_assert(ResultCount == 1); //!!!later mb multiple return values - data array!
		LuaStackToData(*pRetVal, -1);
	}

	UPTR Result = lua_toboolean(l, -1) ? Success : Failure;
	lua_pop(l, ResultCount);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginClass(const char* Name, const char* BaseClass, UPTR FieldCount)
{
	n_assert2(Name && *Name, "Invalid class name to register");
	n_assert2(CurrClass.IsEmpty(), "Already in class registration process!");
	n_assert2(!CurrObj, "Already in mixing-in process!");

	if (ClassExists(Name)) FAIL;
	if (BaseClass && *BaseClass && !ClassExists(BaseClass) && !LoadClass(BaseClass)) FAIL;

	CurrClass = Name;

	lua_getglobal(l, TBL_CLASSES);

	// Create class table
	lua_createtable(l, 0, FieldCount ? FieldCount : 4);

	// Setup base class
	if (BaseClass && *BaseClass)
	{
		lua_getfield(l, -2, BaseClass);
		lua_setmetatable(l, -2);
	}

	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginExistingClass(const char* Name)
{
	n_assert2(Name && *Name, "Invalid class name to register");
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, Name);
	if (!lua_istable(l, -1))
	{
		lua_pop(l, 2);
		FAIL;
	}
	CurrClass = Name;
	OK;
}
//---------------------------------------------------------------------

void CScriptServer::EndClass(bool IsScriptObjectSubclass)
{
	n_assert(CurrClass.IsValid());

	if (IsScriptObjectSubclass)
	{
		// The only way to access cpp_ptr and non-lua fields
		ScriptSrv->ExportCFunction("__index", CScriptObject_Index);
		ScriptSrv->ExportCFunction("__newindex", CScriptObject_NewIndex);
	}
	else
	{
		lua_pushstring(l, "__index");
		lua_rawget(l, -2);
		bool SelfIndex = lua_isnil(l, -1);
		lua_pop(l, 1);
		if (SelfIndex)
		{
			lua_pushstring(l, "__index");
			lua_pushvalue(l, -2);
			lua_rawset(l, -3);
		}
	}

	lua_setfield(l, -2, CurrClass.CStr());
	lua_pop(l, 1); // Classes table
	CurrClass = nullptr;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginMixin(CScriptObject* pObj)
{
	n_assert2(pObj, "Invalid object for mixin");
	n_assert2(!CurrObj, "Already in mixing-in process!");
	n_assert2(CurrClass.IsEmpty(), "Already in class registration process!");
	CurrObj = pObj;
	return PlaceObjectOnStack(pObj->GetName().CStr(), pObj->GetTable().CStr());
}
//---------------------------------------------------------------------

void CScriptServer::EndMixin()
{
	n_assert(CurrObj);
	lua_pop(l, 1);
	CurrObj = nullptr;
}
//---------------------------------------------------------------------

void CScriptServer::ExportCFunction(const char* Name, lua_CFunction Function)
{
	lua_pushcfunction(l, Function);
	lua_setfield(l, -2, Name); //???rawset?
}
//---------------------------------------------------------------------

void CScriptServer::ExportIntegerConst(const char* Name, int Value)
{
	lua_pushinteger(l, Value);
	lua_setfield(l, -2, Name); //???rawset?
}
//---------------------------------------------------------------------

void CScriptServer::ClearField(const char* Name)
{
	lua_pushnil(l);
	lua_setfield(l, -2, Name); //???rawset?
}
//---------------------------------------------------------------------

bool CScriptServer::LoadClass(const char* Name)
{
	n_assert2(Name, "Invalid class name to register");

	//!!!use custom format for compiled class, because CDataBuffer is copied during read! Or solve this problem!
	Data::PParams ClassDesc = ParamsUtils::LoadParamsFromPRM((CString("ScriptClasses:") + Name + ".cls").CStr());
	if (!ClassDesc) FAIL;

	const CString& BaseClass = ClassDesc->Get<CString>(CStrID("Base"), CString::Empty);
	if (!BeginClass(Name, BaseClass.IsValid() ? BaseClass.CStr() : nullptr)) FAIL;

	const char* pData = nullptr;
	UPTR Size = 0;

	Data::CParam* pCodePrm;
	if (ClassDesc->TryGet(pCodePrm, CStrID("Code")))
	{
		if (pCodePrm->IsA<Data::CBufferMalloc>())
		{
			const Data::CBufferMalloc& Code = pCodePrm->GetValue<Data::CBufferMalloc>();
			pData = (const char*)Code.GetConstPtr();
			Size = Code.GetSize();
		}
		else if (pCodePrm->IsA<CString>())
		{
			const CString& Code = pCodePrm->GetValue<CString>();
			pData = Code.CStr();
			Size = Code.GetLength();
		}
	}

	if (pData && Size)
	{
		if (luaL_loadbuffer(l, pData, Size, Name) != 0)
		{
			Sys::Log("Error parsing script for class %s: %s\n", Name, lua_tostring(l, -1));
			if (pCodePrm->IsA<CString>()) Sys::Log("Script is: %s\n", pData);
			lua_pop(l, 2);
			FAIL; // return Error_Scripting_Parsing;
		}
		
		lua_pushvalue(l, -2);
		//lua_setfenv(l, -2);

		if (lua_pcall(l, 0, 0, 0))
		{
			Sys::Log("Error running script for class %s: %s\n", Name, lua_tostring(l, -1));
			if (pCodePrm->IsA<CString>()) Sys::Log("Script is: %s\n", pData);
			lua_pop(l, 2); // Error msg, class table
			FAIL;
		}
	}
	
	EndClass(BaseClass.IsValid());

	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::ClassExists(const char* Name)
{
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, Name);
	bool Result = lua_istable(l, -1);
	lua_pop(l, 2);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptServer::CreateObject(CScriptObject& Obj, const char* LuaClassName)
{
	n_assert(Obj.Name.IsValid() && Obj.Table != TBL_CLASSES && !ObjectExists(Obj.Name.CStr(), Obj.Table.CStr()));
	n_assert(LuaClassName && *LuaClassName && (ClassExists(LuaClassName) || LoadClass(LuaClassName)));

	// Create object table
	lua_createtable(l, 0, 4);

	// Setup class of this object
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, LuaClassName);
	n_assert2(lua_istable(l, -1), "Requested Lua class does not exist");
	lua_setmetatable(l, -3);
	lua_pop(l, 1);

	// Setup object's system fields
	lua_pushstring(l, "cpp_ptr");
	lua_pushlightuserdata(l, &Obj);
	lua_rawset(l, -3);
	lua_pushstring(l, "this");
	lua_pushvalue(l, -2);
	lua_rawset(l, -3);

	// Call constructor //!!!call also parent constructors!
	lua_getfield(l, -1, LuaClassName); //???rename to OnCreate / Construct?
	if (lua_isfunction(l, -1)) 
	{
		lua_pushvalue(l, -2);
		//lua_setfenv(l, -2);
		if (lua_pcall(l, 0, 0, 0))
		{
			Sys::Log("Error running %s class constructor for %s: %s\n",
				LuaClassName, Obj.Name.CStr(), lua_tostring(l, -1));
			lua_pop(l, 2);
			FAIL;
		}
	}
	else lua_pop(l, 1);

	// Setup containing table if needed and add new object
	if (Obj.Table.IsValid())
	{
		// Get table that contains object. Now search only in globals.
		lua_getglobal(l, Obj.Table.CStr());
		if (lua_isnil(l, -1))
		{
			// Create required table if !exist. Now adds only to globals.
			lua_pop(l, 1);
			lua_createtable(l, 0, INITIAL_OBJ_TABLE_SIZE);
			lua_pushvalue(l, -1);
			lua_setglobal(l, Obj.Table.CStr());
		}
		else if (!lua_istable(l, -1))
		{
			//???assert?
			Sys::Log("Error: table name \"%s\" is used by other non-table object\n", Obj.Table.CStr());
			lua_pop(l, 2);
			FAIL;
		}

		lua_pushvalue(l, -2);
		lua_setfield(l, -2, Obj.Name.CStr());
		lua_pop(l, 2);
	}
	else lua_setglobal(l, Obj.Name.CStr());

	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::CreateInterface(const char* Name, const char* TablePath, const char* LuaClassName, void* pCppPtr)
{
	if (!pCppPtr) FAIL;

	if (!PlaceOnStack(TablePath, true)) FAIL;

	// Create object and store interface pointer
	void** ppObj = (void**)lua_newuserdata(l, sizeof(pCppPtr));
	*ppObj = pCppPtr;

	// Setup class of this interface
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, LuaClassName);
	n_assert2(lua_istable(l, -1), "Requested Lua class does not exist");
	lua_setmetatable(l, -3);
	lua_pop(l, 1);

	// Set to parent
	lua_setfield(l, -2, Name);
	lua_pop(l, 1);

	OK;
}
//---------------------------------------------------------------------

// Places object to -1 and optionally object's containing table to -2
bool CScriptServer::PlaceObjectOnStack(const char* Name, const char* Table)
{
	if (!l) FAIL;

	//???!!!cache what object is currently on stack? if needed, return immediately, if new, remove old before

	int TableIdx;
	if (Table && *Table)
	{
		// Get table that contains object. Now search only in globals.
		lua_getglobal(l, Table);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			FAIL;
		}
		TableIdx = -1;
	}
	//else TableIdx = LUA_GLOBALSINDEX;

	lua_getfield(l, TableIdx, Name);
	if (lua_istable(l, -1) || lua_isuserdata(l, -1)) OK;
	else
	{
		//lua_pop(l, (TableIdx == LUA_GLOBALSINDEX) ? 1 : 2);
		FAIL;
	}
}
//---------------------------------------------------------------------

// Places any named Lua var on the top of the stack
bool CScriptServer::PlaceOnStack(const char* FullPath, bool Create)
{
	if (!l || !FullPath) FAIL;

	//char Buffer[512];
	Data::CStringTokenizer StrTok(FullPath); //, Buffer, 512);

	lua_getglobal(l, "_G");

	const char* pTok;
	while (pTok = StrTok.GetNextToken('.'))
	{
		if (!*pTok) continue;

		lua_getfield(l, -1, pTok);
		if (lua_isnil(l, -1) && Create)
		{
			lua_pop(l, 1);				// Remove nil
			lua_createtable(l, 0, 2);	// Create new table at -1
			lua_pushvalue(l, -1);		// Copy new table, it is at -1 and -2, parent is at -3
			lua_setfield(l, -3, pTok);	// Set as parent's field
			lua_remove(l, -2);			// Remove parent table
		}
		else if (lua_istable(l, -1) || StrTok.IsLast())
		{
			lua_remove(l, -2);			// Remove parent table
		}
		else
		{
			lua_pop(l, 2);				// Remove parent table and value received
			FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

void CScriptServer::RemoveObject(const char* Name, const char* Table)
{
	if (PlaceObjectOnStack(Name, Table))
	{
		lua_pop(l, 1);
		lua_pushnil(l);
		//lua_setfield(l, (Table && *Table) ? -2 : LUA_GLOBALSINDEX, Name);
		if (Table && *Table) lua_pop(l, 1);
	}
}
//---------------------------------------------------------------------

bool CScriptServer::ObjectExists(const char* Name, const char* Table)
{
	if (!PlaceObjectOnStack(Name, Table)) FAIL;
	lua_pop(l, (Table && *Table) ? 2 : 1);
	OK;
}
//---------------------------------------------------------------------

//???universalize?
bool CScriptServer::GetTableFieldsDebug(CArray<CString>& OutFields)
{
	OutFields.Clear();

	if (!l || !lua_istable(l, -1)) FAIL;

	lua_pushnil(l);
	while (lua_next(l, -2))
	{
		CString& New = *OutFields.Reserve(1);
		if (lua_type(l, -2) == LUA_TSTRING) New = lua_tostring(l, -2);
		else if (lua_type(l, -2) == LUA_TNUMBER)
		{
			New = "[";
			New += StringUtils::FromInt((I32)lua_tointeger(l, -2));
			New += "]";
		}
		else
		{
			lua_pop(l, 1);
			continue;
		}

		// Can use lua_typename
		switch (lua_type(l, -1))
		{
			case LUA_TNIL:				New += "(nil)"; break;
			case LUA_TBOOLEAN:			New += " = "; New += StringUtils::FromBool(lua_toboolean(l, -1) != 0); New += " (bool)"; break;
			case LUA_TNUMBER:
			{
				New += " = ";
				//double Value = lua_tonumber(l, -1);
				//int IntValue;
				//lua_number2int(IntValue, Value);
				//if (((double)IntValue) == Value) New += StringUtils::FromInt(IntValue);
				//else New += StringUtils::FromFloat((float)Value);
				New += " (number)";
				break;
			}
			case LUA_TSTRING:			New += " = "; New += lua_tostring(l, -1); New += " (string)"; break;
			case LUA_TTABLE:			New += "(table)"; break;
			case LUA_TFUNCTION:			New += "(function)"; break;
			case LUA_TUSERDATA:			New += "(lightuserdata)"; break;
			case LUA_TLIGHTUSERDATA:	New += "(userdata)"; break;
		}

		lua_pop(l, 1);
	}

	lua_pop(l, 1); // Remove listed table, for now forced
	OK;
}
//---------------------------------------------------------------------

}
