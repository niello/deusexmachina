#include "LuaTextResolver.h"

namespace /*DEM::*/Data
{

bool CLuaTextResolver::ResolveToken(std::string_view In, CStringAppender Out)
{
	const std::string Code = "return " + std::string(In.substr(1, In.size() - 2));
	auto Result = _Env ? _State.script(Code, _Env) : _State.script(Code);
	if (!Result.valid()) return false;

	const auto Type = Result.get_type();
	if (Type == sol::type::none || Type == sol::type::nil) return false;

	DEM::Scripting::ObjectToString(Result.get<sol::object>(), _State, [&Out](std::string_view Str) { Out.Append(Str); });
	return true;
}
//---------------------------------------------------------------------

}
