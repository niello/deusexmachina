#include <Scripting/ScriptServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Quests/QuestManager.h>

// Script Interface for the Quest system

namespace SI
{
using namespace Story;

int CQuestMgr_StartQuest(lua_State* l)
{
	//args: quest name, [task name = empty]

	int ArgCount = lua_gettop(l);

	if (!ArgCount || !lua_isstring(l, 1))
	{
		lua_settop(l, 0);
		return 0;
	}

	CStrID Task = (ArgCount > 1) ? CStrID(lua_tostring(l, 2)) : CStrID::Empty;
	QuestMgr->StartQuest(CStrID(lua_tostring(l, 1)), Task);
	return 0;
}
//---------------------------------------------------------------------

int CQuestMgr_CompleteQuest(lua_State* l)
{
	//args: quest name, [task name = empty]
	
	int ArgCount = lua_gettop(l);

	if (!ArgCount || !lua_isstring(l, 1))
	{
		lua_settop(l, 0);
		return 0;
	}

	CStrID Task = (ArgCount > 1) ? CStrID(lua_tostring(l, 2)) : CStrID::Empty;
	QuestMgr->CompleteQuest(CStrID(lua_tostring(l, 1)), Task);
	return 0;
}
//---------------------------------------------------------------------

int CQuestMgr_GetQuestStatus(lua_State* l)
{
	//args: quest name, [task name = empty]
	
	int ArgCount = lua_gettop(l);

	if (!ArgCount || !lua_isstring(l, 1))
	{
		lua_settop(l, 0);
		return 0;
	}

	CStrID Task = (ArgCount > 1) ? CStrID(lua_tostring(l, 2)) : CStrID::Empty;
	lua_pushinteger(l, (int)QuestMgr->GetQuestStatus(CStrID(lua_tostring(l, 1)), Task));
	return 1;
}
//---------------------------------------------------------------------

bool RegisterQuestSystem()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 7);

	ScriptSrv->ExportCFunction("StartQuest", CQuestMgr_StartQuest);
	ScriptSrv->ExportCFunction("CompleteQuest", CQuestMgr_CompleteQuest);
	ScriptSrv->ExportCFunction("GetQuestStatus", CQuestMgr_GetQuestStatus);

	ScriptSrv->ExportIntegerConst("QSNo", CQuest::No);
	ScriptSrv->ExportIntegerConst("QSOpened", CQuest::Opened);
	ScriptSrv->ExportIntegerConst("QSDone", CQuest::Done);
	ScriptSrv->ExportIntegerConst("QSFailed", CQuest::Failed);

	lua_setglobal(l, "QuestMgr");

	OK;
}
//---------------------------------------------------------------------

}