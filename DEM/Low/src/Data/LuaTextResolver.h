#pragma once
#include <Data/TextResolver.h>
#include <Scripting/SolLow.h>

// A text resolver which substitutes values from Lua expressions

namespace /*DEM::*/Data
{

class CLuaTextResolver : public ITextResolver
{
public:

	sol::state&      _State;
	sol::environment _Env;

	CLuaTextResolver(sol::state& State, sol::environment Env = {}) : _State(State), _Env(Env) {}

	virtual bool ResolveToken(std::string_view In, CStringAppender Out) override;
};

inline PTextResolver CreateTextResolver(sol::state& State)
{
	return new CLuaTextResolver(State);
}
//---------------------------------------------------------------------

inline PTextResolver CreateTextResolver(sol::state& State, sol::environment Env)
{
	return new CLuaTextResolver(State, Env);
}
//---------------------------------------------------------------------

}
