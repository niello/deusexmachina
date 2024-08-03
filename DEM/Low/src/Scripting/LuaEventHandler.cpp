#include "LuaEventHandler.h"
#include <Events/Event.h>

namespace Events
{

bool CLuaEventHandler::Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event)
{
	const auto pEvent = Event.As<const CEvent>();
	if (!pEvent) return false;

	if (_Fn.valid())
	{
		auto Result = _Fn(pEvent);
		if (!Result.valid())
		{
			::Sys::Error(Result.get<sol::error>().what());
			return false;
		}

		const auto Type = Result.get_type();
		return Type != sol::type::nil && Type != sol::type::none && Result;
	}

	return false;
}
//---------------------------------------------------------------------

}
