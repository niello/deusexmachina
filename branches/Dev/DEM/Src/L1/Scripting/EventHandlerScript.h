#pragma once
#ifndef __DEM_L1_EVENT_HANDLER_SCRIPT_H__
#define __DEM_L1_EVENT_HANDLER_SCRIPT_H__

#include <Events/EventHandler.h>

// Event handler that calls scripted function (global or owned by object)

namespace Scripting
{
	class CScriptObject;
}

namespace Events
{
using namespace Scripting;

class CEventHandlerScript: public CEventHandler
{
private:

	CScriptObject*	pObject; //???weak ref?
	nString			Func;

public:

	CEventHandlerScript(CScriptObject* Obj, const nString& FuncName, ushort _Priority = Priority_Default): CEventHandler(_Priority), pObject(Obj), Func(FuncName) {}

	virtual bool operator()(const CEventBase& Event);

	CScriptObject*	GetObj() const { return pObject; }
	const nString&	GetFunc() const { return Func; }
};

}

#endif