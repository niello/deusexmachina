#include <Scripting/ScriptServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Factions/Faction.h>

// Script Interface for the CFaction class

namespace SI
{

int CFaction_IsMember(lua_State* l)
{
	//args: 'this' userdata, character StrID
	//ret:	bool
	RPG::CFaction* pFaction = *(RPG::CFaction**)lua_touserdata(l, 1);
	n_assert_dbg(pFaction);
	lua_pushboolean(l, pFaction && pFaction->IsMember(CStrID(lua_tostring(l, 2))));
	return 1;
}
//---------------------------------------------------------------------

int CFaction_IsLeader(lua_State* l)
{
	//args: 'this' userdata, character StrID
	//ret:	bool
	RPG::CFaction* pFaction = *(RPG::CFaction**)lua_touserdata(l, 1);
	n_assert_dbg(pFaction);
	lua_pushboolean(l, pFaction && pFaction->IsLeader(CStrID(lua_tostring(l, 2))));
	return 1;
}
//---------------------------------------------------------------------

int CFaction_GetLeader(lua_State* l)
{
	//args: 'this' userdata
	//ret:	string
	RPG::CFaction* pFaction = *(RPG::CFaction**)lua_touserdata(l, 1);
	n_assert_dbg(pFaction);
	lua_pushstring(l, pFaction ? pFaction->GetLeader().CStr() : nullptr);
	return 1;
}
//---------------------------------------------------------------------

bool RegisterClassCFaction()
{
	if (!ScriptSrv->BeginClass("CFaction")) FAIL;
	ScriptSrv->ExportCFunction("IsMember", CFaction_IsMember);
	ScriptSrv->ExportCFunction("IsLeader", CFaction_IsLeader);
	ScriptSrv->ExportCFunction("GetLeader", CFaction_GetLeader);
	ScriptSrv->EndClass(false);
	OK;
}
//---------------------------------------------------------------------

}