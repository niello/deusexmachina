#pragma once
#ifndef __DEM_L1_SCRIPT_SERVER_H__
#define __DEM_L1_SCRIPT_SERVER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <StdDEM.h>
#include <Events/EventsFwd.h>

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

class CScriptServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CScriptServer);

private:

	lua_State*		l; // Yes, just 'l'. It's very convinient when writing & reading tons of Lua-related code.

	nString			CurrClass;
	CScriptObject*	CurrObj;

public:

	CScriptServer();
	~CScriptServer();

	lua_State*	GetLuaState() const { return l; }

	int			DataToLuaStack(const Data::CData& Data);
	bool		LuaStackToData(Data::CData& Result, int StackIdx, lua_State* L = NULL);

	EExecStatus	RunScriptFile(const nString& FileName);
	EExecStatus	RunScript(LPCSTR Buffer, DWORD Length = -1, Data::CData* pRetVal = NULL);

	EExecStatus	PerformCall(int ArgCount, Data::CData* pRetVal = NULL, LPCSTR pDbgName = "<UNKNOWN>");
	
	//EExecStatus		RunFunction(LPCSTR pFuncName, int ArgsOnStack = 0);
	//EExecStatus		RunFunction(LPCSTR pFuncName, LPCSTR LuaArg);
	//EExecStatus		RunFunction(LPCSTR pFuncName, const CArray<LPCSTR>& LuaArgs);
	//EExecStatus		RunFunction(LPCSTR pFuncName, PParams Args = NULL);

	// Class registration, Mixing-in
	bool		BeginClass(LPCSTR Name, LPCSTR BaseClass = NULL, DWORD FieldCount = 0);
	bool		BeginExistingClass(LPCSTR Name);
	void		EndClass(bool IsScriptObjectSubclass);
	bool		BeginMixin(CScriptObject* pObj);
	void		EndMixin();
	void		ExportCFunction(LPCSTR Name, lua_CFunction Function);
	void		ExportIntegerConst(LPCSTR Name, int Value);

	bool		LoadClass(const nString& Name);
	bool		ClassExists(LPCSTR Name);

	//!!!can add functions to subscribe global functions to events!
	
	bool		CreateObject(CScriptObject& Obj, LPCSTR LuaClassName = "CScriptObject");
	bool		CreateInterface(LPCSTR Name, LPCSTR TablePath, LPCSTR LuaClassName, void* pCppPtr);
	bool		PlaceObjectOnStack(LPCSTR Name, LPCSTR Table = NULL);
	bool		PlaceOnStack(LPCSTR FullPath, bool Create = false);
	void		RemoveObject(LPCSTR Name, LPCSTR Table = NULL);
	bool		ObjectExists(LPCSTR Name, LPCSTR Table = NULL);

	bool		GetTableFieldsDebug(CArray<nString>& OutFields);
};

}

#endif
