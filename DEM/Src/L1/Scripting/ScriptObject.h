#pragma once
#ifndef __DEM_L1_SCRIPT_OBJ_H__
#define __DEM_L1_SCRIPT_OBJ_H__

#include <Core/RefCounted.h>
#include <util/narray.h>
#include <Events/Subscription.h>
#include "Scripting.h"

#undef RegisterClass

// Script object is a base for all scripted objects. It can interact with scripts in both directions.

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

class CScriptObject: public Core::CRefCounted
{
	__DeclareClass(CScriptObject);

protected:

	nString					Name;
	nString					Table;
	nArray<Events::PSub>	Subscriptions;

	CScriptObject(): DBSaveLoadEnabled(false) {}

	bool		PrepareToLuaCall(LPCSTR pFuncName) const;
	EExecStatus	RunFunctionInternal(LPCSTR pFuncName, int ArgCount, Data::CData* pRetVal) const;

public:

	friend class CScriptServer;

	bool DBSaveLoadEnabled;

	CScriptObject(LPCSTR ObjName, LPCSTR TableName = NULL):
		Name(ObjName), Table(TableName), DBSaveLoadEnabled(false) {}
	virtual ~CScriptObject();

	static CScriptObject* GetFromStack(lua_State* l, int StackIdx);

	bool			Init(LPCSTR LuaClassName = "CScriptObject");

	EExecStatus		LoadScriptFile(const nString& FileName);
	EExecStatus		LoadScript(LPCSTR Buffer, DWORD Length);

	bool			SaveFields(Data::CParams& Dest);
	bool			LoadFields(const Data::CParams& Src);

	EExecStatus		RunFunction(LPCSTR pFuncName, Data::CData* pRetVal = NULL) const;
	EExecStatus		RunFunction(LPCSTR pFuncName, LPCSTR LuaArg, Data::CData* pRetVal = NULL) const;
	EExecStatus		RunFunctionData(LPCSTR FuncName, const Data::CData& Arg, Data::CData* pRetVal = NULL) const;
	//int				RunFunction(LPCSTR pFuncName, const nArray<LPCSTR>& LuaArgs);
	//int				RunFunction(LPCSTR pFuncName, CDataArray);????

	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, Events::CEventDispatcher* pDisp, ushort Priority);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, const Events::CEventDispatcher* pDisp);
	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority = Events::Priority_Default);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName);

	const nString&	GetName() const { return Name; }
	const nString&	GetTable() const { return Table; }
	nString			GetFullName() const { return Table.IsValid() ? Table + "." + Name : Name; }
	void			SetName(const char* NewName);

	//???implement?
	virtual int		GetField(LPCSTR Key) const { return 0; } //???is always const?
	virtual bool	SetField(LPCSTR Key, const Data::CData& Value) { FAIL; }
};

typedef Ptr<CScriptObject> PScriptObject;

inline EExecStatus CScriptObject::RunFunction(LPCSTR pFuncName, Data::CData* pRetVal) const
{
	return PrepareToLuaCall(pFuncName) ? RunFunctionInternal(pFuncName, 0, pRetVal) : Error;
}
//---------------------------------------------------------------------

int CScriptObject_Index(lua_State* l);
int CScriptObject_NewIndex(lua_State* l);
int CScriptObject_SubscribeEvent(lua_State* l);
int CScriptObject_UnsubscribeEvent(lua_State* l);

}

#endif
