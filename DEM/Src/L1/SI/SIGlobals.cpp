#include <Scripting/ScriptServer.h>
#include <StdDEM.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

// Script Interface for global functions and constants

namespace SI
{

//using namespace Scripting;

int Print(lua_State* l)
{
	//???concat args if > 1?
	n_printf(lua_tostring(l, 1));
	return 0;
}
//---------------------------------------------------------------------

bool RegisterGlobals()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_register(l, "print", Print);

	OK;
}
//---------------------------------------------------------------------

}