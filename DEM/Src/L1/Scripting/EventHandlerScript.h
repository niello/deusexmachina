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
using namespace Scripting;

class CEventHandlerScript: public CEventHandler
{
private:

	CScriptObject*	pObject; //???weak ref?
	CString			Func;

public:

	CEventHandlerScript(CScriptObject* Obj, const CString& FuncName, ushort _Priority = Priority_Default): CEventHandler(_Priority), pObject(Obj), Func(FuncName) {}

	virtual bool operator()(const CEventBase& Event);

	CScriptObject*	GetObj() const { return pObject; }
	const CString&	GetFunc() const { return Func; }
};

}

#endif