#pragma once
#ifndef __DEM_L1_EVENT_HANDLER_SCRIPT_H__
#define __DEM_L1_EVENT_HANDLER_SCRIPT_H__

#include <Events/EventHandler.h>

// Event handler that calls scripted function (global or owned by object).
// For parametric events only, at least for now.

namespace Scripting
{
	class CScriptObject;
}

namespace Events
{

class CEventHandlerScript: public CEventHandler
{
private:

	Scripting::CScriptObject*	pObject; //???strong or weak ref?
	CString						Func;

public:

	CEventHandlerScript(Scripting::CScriptObject* Obj, const char* pFuncName, ushort _Priority = Priority_Default): CEventHandler(_Priority), pObject(Obj), Func(pFuncName) {}

	virtual bool				Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event);

	Scripting::CScriptObject*	GetObj() const { return pObject; }
	const CString&				GetFunc() const { return Func; }
};

}

#endif