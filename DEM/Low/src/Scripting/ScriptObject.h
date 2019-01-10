#pragma once
#ifndef __DEM_L1_SCRIPT_OBJ_H__
#define __DEM_L1_SCRIPT_OBJ_H__

#include <Core/Object.h>
#include <Data/Array.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>

// Script object is a base for all scripted objects. It can interact with scripts in both directions.

//???!!!in script srv cache object on stack?!

const UPTR Error_Scripting_NoObject = Error + 1;
const UPTR Error_Scripting_NoFunction = Error + 2;
const UPTR Error_Scripting_Parsing = Error + 3;

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

	UPTR	PrepareToLuaCall(const char* pFuncName) const;
	UPTR	RunFunctionInternal(const char* pFuncName, int ArgCount, Data::CData* pRetVal) const;

public:

	friend class CScriptServer;

	CScriptObject(const char* ObjName, const char* TableName = nullptr): Name(ObjName), Table(TableName) {}
	virtual ~CScriptObject();

	static CScriptObject* GetFromStack(lua_State* l, int StackIdx);

	bool			Init(const char* LuaClassName = "CScriptObject");

	UPTR			LoadScriptFile(const char* pFileName);
	UPTR			LoadScript(const char* Buffer, UPTR Length);

	UPTR			RunFunction(const char* pFuncName, Data::CData* pRetVal = NULL) const;
	UPTR			RunFunction(const char* pFuncName, const char* ArgLuaGlobal, Data::CData* pRetVal = NULL) const;
	UPTR			RunFunction(const char* pFuncName, Data::CData* Args, UPTR ArgCount, Data::CData* pRetVal = NULL) const;
	UPTR			RunFunctionOneArg(const char* FuncName, const Data::CData& Arg, Data::CData* pRetVal = NULL) const;

	bool			SubscribeEvent(CStrID EventID, const char* HandlerFuncName, Events::CEventDispatcher* pDisp, U16 Priority);
	void			UnsubscribeEvent(CStrID EventID, const char* HandlerFuncName, const Events::CEventDispatcher* pDisp);
	bool			SubscribeEvent(CStrID EventID, const char* HandlerFuncName, U16 Priority = Events::Priority_Default);
	void			UnsubscribeEvent(CStrID EventID, const char* HandlerFuncName);

	const CString&	GetName() const { return Name; }
	const CString&	GetTable() const { return Table; }
	CString			GetFullName() const { return Table.IsValid() ? Table + "." + Name : Name; }
	void			SetName(const char* NewName);

	//???implement?
	virtual int		GetField(const char* Key) const { return 0; } //???is always const?
	virtual bool	SetField(const char* Key, const Data::CData& Value) { FAIL; }
};

typedef Ptr<CScriptObject> PScriptObject;

inline UPTR CScriptObject::RunFunction(const char* pFuncName, Data::CData* pRetVal) const
{
	UPTR Res = PrepareToLuaCall(pFuncName);
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
