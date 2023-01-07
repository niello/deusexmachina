#include "LuaEventHandler.h"
#include <Events/Event.h>

namespace Events
{

bool CLuaEventHandler::Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event)
{
	// TODO: pass event objet to Lua as is?!
	const auto pEvent = Event.As<const CEvent>();
	if (!pEvent) return false;

	//!!!TODO: pass event args to the handler!

	if (_Fn.valid())
	{
		auto Result = _Fn();
		if (!Result.valid())
		{
			sol::error Error = Result;
			::Sys::Error(Error.what());
			return false;
		}

		const auto Type = Result.get_type();
		return Type != sol::type::nil && Type != sol::type::none && Result;
	}

	return false;
}
//---------------------------------------------------------------------

}
