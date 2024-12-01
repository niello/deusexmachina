#pragma once
#include <Events/EventHandler.h>
#include <Scripting/SolLow.h>

// Event handler that calls Lua function.
// For parametric events only, at least for now.

namespace Events
{

class CLuaEventHandler: public CEventHandler
{
private:

	sol::function _Fn;

public:

	CLuaEventHandler(sol::function&& Fn, CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default): CEventHandler(d, e, _Priority), _Fn(std::move(Fn)) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event);

	const sol::function& GetFunction() const { return _Fn; }
};

}
