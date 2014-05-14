#pragma once
#ifndef __DEM_L1_SCRIPT_OBJ_H__
#define __DEM_L1_SCRIPT_OBJ_H__

#include <Core/Object.h>
#include <Data/Array.h>
#include <Events/Subscription.h>

#undef RegisterClass

// Script object is a base for all scripted objects. It can interact with scripts in both directions.

//???!!!in script srv cache object on stack?!

const DWORD Error_Scripting_NoObject = Error + 1;
const DWORD Error_Scripting_NoFunction = Error + 2;
const DWORD Error_Scripting_Parsing = Error + 3;

struct lua_State;

namespace Events
{
	class CEventDispatcher;
	typedef Ptr<CSubscription> PSub;
}

namespace Data
{
	class CData;
	class CParams;
}

namespace Scripting
{

class CScriptObject: public Core::CObject
{
	__DeclareClass(CScriptObject);

protected:

	CString					Name;
	CString					Table;
	CArray<Events::PSub>	Subscriptions;

	CScriptObject() {}

	DWORD	PrepareToLuaCall(LPCSTR pFuncName) const;
	DWORD	RunFunctionInternal(LPCSTR pFuncName, int ArgCount, Data::CData* pRetVal) const;

public:

	friend class CScriptServer;

	CScriptObject(LPCSTR ObjName, LPCSTR TableName = NULL): Name(ObjName), Table(TableName) {}
	virtual ~CScriptObject();

	static CScriptObject* GetFromStack(lua_State* l, int StackIdx);

	bool			Init(LPCSTR LuaClassName = "CScriptObject");

	DWORD			LoadScriptFile(const CString& FileName);
	DWORD			LoadScript(LPCSTR Buffer, DWORD Length);

	DWORD			RunFunction(LPCSTR pFuncName, Data::CData* pRetVal = NULL) const;
	DWORD			RunFunction(LPCSTR pFuncName, LPCSTR ArgLuaGlobal, Data::CData* pRetVal = NULL) const;
	DWORD			RunFunction(LPCSTR pFuncName, Data::CData* Args, DWORD ArgCount, Data::CData* pRetVal = NULL) const;
	DWORD			RunFunctionOneArg(LPCSTR FuncName, const Data::CData& Arg, Data::CData* pRetVal = NULL) const;

	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, Events::CEventDispatcher* pDisp, ushort Priority);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, const Events::CEventDispatcher* pDisp);
	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority = Events::Priority_Default);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName);

	const CString&	GetName() const { return Name; }
	const CString&	GetTable() const { return Table; }
	CString			GetFullName() const { return Table.IsValid() ? Table + "." + Name : Name; }
	void			SetName(const char* NewName);

	//???implement?
	virtual int		GetField(LPCSTR Key) const { return 0; } //???is always const?
	virtual bool	SetField(LPCSTR Key, const Data::CData& Value) { FAIL; }
};

typedef Ptr<CScriptObject> PScriptObject;

inline DWORD CScriptObject::RunFunction(LPCSTR pFuncName, Data::CData* pRetVal) const
{
	DWORD Res = PrepareToLuaCall(pFuncName);
	if (ExecResultIsError(Res)) return Res;
	return RunFunctionInternal(pFuncName, 0, pRetVal);
}
//---------------------------------------------------------------------

int CScriptObject_Index(lua_State* l);
int CScriptObject_NewIndex(lua_State* l);
int CScriptObject_SubscribeEvent(lua_State* l);
int CScriptObject_UnsubscribeEvent(lua_State* l);

}

#endif
