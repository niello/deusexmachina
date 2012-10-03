#pragma once
#ifndef __DEM_L1_SCRIPT_OBJ_H__
#define __DEM_L1_SCRIPT_OBJ_H__

#include <Core/RefCounted.h>
#include <util/narray.h>
#include <DB/AttrID.h>
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
}

namespace DB
{
	class CDatabase;
}

namespace Attr
{
	DeclareString(LuaObjName);
	DeclareString(LuaFieldName);
	DeclareAttr(LuaValue);
}

namespace Scripting
{
using namespace Events;
using namespace Data;

class CScriptObject: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CScriptObject);

protected:

	nString			Name;
	nString			Table;
	nArray<PSub>	Subscriptions;

	CScriptObject(): DBSaveLoadEnabled(false) {}

	bool		PrepareToLuaCall(LPCSTR pFuncName) const;
	EExecStatus	RunFunctionInternal(LPCSTR pFuncName, int ArgCount, CData* pRetVal) const;

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

	bool			SaveFields(DB::CDatabase* pDB);
	bool			LoadFields(const DB::CDatabase* pDB);
	void			ClearFields(DB::CDatabase* pDB);
	static void		ClearFieldsDeffered(DB::CDatabase* pDB, const nString FullObjName);

	EExecStatus		RunFunction(LPCSTR pFuncName, CData* pRetVal = NULL) const;
	EExecStatus		RunFunction(LPCSTR pFuncName, LPCSTR LuaArg, CData* pRetVal = NULL) const;
	EExecStatus		RunFunctionData(LPCSTR FuncName, const CData& Arg, CData* pRetVal = NULL) const;
	//int				RunFunction(LPCSTR pFuncName, const nArray<LPCSTR>& LuaArgs);
	//int				RunFunction(LPCSTR pFuncName, CDataArray);????

	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, CEventDispatcher* pDisp, ushort Priority);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, const CEventDispatcher* pDisp);
	bool			SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority = Priority_Default);
	void			UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName);

	const nString&	GetName() const { return Name; }
	const nString&	GetTable() const { return Table; }
	nString			GetFullName() const { return Table.IsValid() ? Table + "." + Name : Name; }
	void			SetName(const char* NewName);

	virtual int		GetField(LPCSTR Key) const { return 0; } //???is always const?
	virtual bool	SetField(LPCSTR Key, const CData& Value) { FAIL; }
};

RegisterFactory(CScriptObject);

typedef Ptr<CScriptObject> PScriptObject;

inline EExecStatus CScriptObject::RunFunction(LPCSTR pFuncName, CData* pRetVal) const
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
