#pragma once
#ifndef __DEM_L1_SCRIPT_SERVER_H__
#define __DEM_L1_SCRIPT_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/Array.h>

// Script server is a central point for script objects creation, registration and script running
// Should store script interpreter, loader from file etc
// Current implementation uses Lua 5.1.4 patch 4 as a scripting language

//???cpp_ptr as closure var?

struct lua_State;
typedef int (*lua_CFunction) (lua_State *L);

namespace Data
{
	class CData;
}

namespace Scripting
{
typedef Ptr<class CScriptObject> PScriptObject;

#define ScriptSrv Scripting::CScriptServer::Instance()

class CScriptServer
{
	__DeclareSingleton(CScriptServer);

private:

	lua_State*		l; // Yes, just 'l'. It's very convinient when writing & reading tons of Lua-related code.

	CString			CurrClass;
	CScriptObject*	CurrObj;

public:

	CScriptServer();
	~CScriptServer();

	lua_State*	GetLuaState() const { return l; }

	int			DataToLuaStack(const Data::CData& Data);
	bool		LuaStackToData(Data::CData& Result, int StackIdx);

	DWORD		RunScriptFile(const char* pFileName);
	DWORD		RunScript(const char* Buffer, DWORD Length = -1, Data::CData* pRetVal = NULL);

	DWORD		PerformCall(int ArgCount, Data::CData* pRetVal = NULL, const char* pDbgName = "<UNKNOWN>");
	
	//DWORD		RunFunction(const char* pFuncName, int ArgsOnStack = 0);
	//DWORD		RunFunction(const char* pFuncName, const char* ArgLuaGlobal);
	//DWORD		RunFunction(const char* pFuncName, const CArray<const char*>& LuaArgs);
	//DWORD		RunFunction(const char* pFuncName, PParams Args = NULL);

	// Class registration, Mixing-in
	bool		BeginClass(const char* Name, const char* BaseClass = NULL, DWORD FieldCount = 0);
	bool		BeginExistingClass(const char* Name);
	void		EndClass(bool IsScriptObjectSubclass);
	bool		BeginMixin(CScriptObject* pObj);
	void		EndMixin();
	void		ExportCFunction(const char* Name, lua_CFunction Function);
	void		ExportIntegerConst(const char* Name, int Value);
	void		ClearField(const char* Name);

	bool		LoadClass(const char* Name);
	bool		ClassExists(const char* Name);

	//!!!can add functions to subscribe global functions to events!
	
	bool		CreateObject(CScriptObject& Obj, const char* LuaClassName = "CScriptObject");
	bool		CreateInterface(const char* Name, const char* TablePath, const char* LuaClassName, void* pCppPtr);
	bool		PlaceObjectOnStack(const char* Name, const char* Table = NULL);
	bool		PlaceOnStack(const char* FullPath, bool Create = false);
	void		RemoveObject(const char* Name, const char* Table = NULL);
	bool		ObjectExists(const char* Name, const char* Table = NULL);

	bool		GetTableFieldsDebug(CArray<CString>& OutFields);
};

}

#endif
