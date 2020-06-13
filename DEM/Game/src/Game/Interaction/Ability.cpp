#include "Ability.h"
#include <System/System.h>

namespace DEM::Game
{

static int luaB_print(lua_State *L)
{
	const int n = lua_gettop(L); // number of arguments
	for (int i = 1; i <= n; ++i)
	{
		size_t l;
		auto s = lua_tolstring(L, i, &l);
		if (!s) return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) ::Sys::Log("\t");
		::Sys::Log(s);
	}
	::Sys::Log("\n");
	return 0;
}
//---------------------------------------------------------------------

CAbility CAbility::CreateFromParams(const Data::CParams& Params)
{
	CAbility Ability;

	// TODO: read Actions, Icon, Name etc

	sol::state Lua;
	Lua.open_libraries(sol::lib::base);

	Lua["print"] = &luaB_print;

	// TODO: pushes the compiled chunk as a Lua function on top of the stack, need to save anywhere in this Ability's table?
	auto LoadedCondition = Lua.load("print(...)");//"local SelectedActors = ...; return TEST(SelectedActors, 'x')"
	if (LoadedCondition.valid())
		Ability.Condition = LoadedCondition;

	if (Ability.Condition)
	{
		auto Result = Ability.Condition(5, 6);
		if (!Result.valid())
		{
			sol::error Error = Result;
			::Sys::DbgOut(Error.what());
		}
	}

	Ability.Condition = sol::nil;
	if (Ability.Condition)
		Ability.Condition(5, 6);

	return Ability;
}
//---------------------------------------------------------------------

}
