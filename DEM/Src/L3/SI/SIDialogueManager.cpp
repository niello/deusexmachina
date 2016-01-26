#include <Scripting/ScriptServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#include <Dlg/DialogueManager.h>

// Script Interface for the Dialogue system

namespace SI
{

int CDlgMgr_GetDialogueState(lua_State* l)
{
	//args: dlg ID
	//ret:	state as int
	int ArgCount = lua_gettop(l);
	if (!ArgCount || !lua_isstring(l, 1)) lua_pushinteger(l, Story::DlgState_None);
	else lua_pushinteger(l, DlgMgr->GetDialogueState(CStrID(lua_tostring(l, 1))));
	return 1;
}
//---------------------------------------------------------------------

int CDlgMgr_AcceptDialogue(lua_State* l)
{
	//args: dlg ID, target ID
	//ret: bool success
	int ArgCount = lua_gettop(l);
	if (ArgCount > 1 && lua_isstring(l, 1) && lua_isstring(l, 2))
		lua_pushboolean(l, DlgMgr->AcceptDialogue(CStrID(lua_tostring(l, 1)), CStrID(lua_tostring(l, 2))));
	else lua_pushboolean(l, false);
	return 1;
}
//---------------------------------------------------------------------

int CDlgMgr_RejectDialogue(lua_State* l)
{
	//args: dlg ID, target ID
	//ret: bool success
	int ArgCount = lua_gettop(l);
	if (ArgCount > 1 && lua_isstring(l, 1) && lua_isstring(l, 2))
		lua_pushboolean(l, DlgMgr->RejectDialogue(CStrID(lua_tostring(l, 1)), CStrID(lua_tostring(l, 2))));
	else lua_pushboolean(l, false);
	return 1;
}
//---------------------------------------------------------------------

int CDlgMgr_CloseDialogue(lua_State* l)
{
	//args: dlg ID
	int ArgCount = lua_gettop(l);
	if (ArgCount > 0 && lua_isstring(l, 1)) DlgMgr->CloseDialogue(CStrID(lua_tostring(l, 1)));
	return 0;
}
//---------------------------------------------------------------------

bool RegisterDlgSystem()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 1);

	ScriptSrv->ExportCFunction("GetDialogueState", CDlgMgr_GetDialogueState);
	ScriptSrv->ExportCFunction("AcceptDialogue", CDlgMgr_AcceptDialogue);
	ScriptSrv->ExportCFunction("RejectDialogue", CDlgMgr_RejectDialogue);
	ScriptSrv->ExportCFunction("CloseDialogue", CDlgMgr_CloseDialogue);

	ScriptSrv->ExportIntegerConst("DlgState_None", Story::DlgState_None);
	ScriptSrv->ExportIntegerConst("DlgState_Requested", Story::DlgState_Requested);
	ScriptSrv->ExportIntegerConst("DlgState_InNode", Story::DlgState_InNode);
	ScriptSrv->ExportIntegerConst("DlgState_Waiting", Story::DlgState_Waiting);
	ScriptSrv->ExportIntegerConst("DlgState_InLink", Story::DlgState_InLink);
	ScriptSrv->ExportIntegerConst("DlgState_Finished", Story::DlgState_Finished);
	ScriptSrv->ExportIntegerConst("DlgState_Aborted", Story::DlgState_Aborted);

	lua_setglobal(l, "DlgMgr");

	OK;
}
//---------------------------------------------------------------------

}