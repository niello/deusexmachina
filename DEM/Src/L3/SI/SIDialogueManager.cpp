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
using namespace Story;

int CDlgSysem_IsDialogueActive(lua_State* l)
{
	//args: no
	//ret:	bool
	lua_pushboolean(l, DlgMgr->IsDialogueActive());
	return 1;
}
//---------------------------------------------------------------------

bool RegisterDlgSystem()
{
	lua_State* l = ScriptSrv->GetLuaState();

	lua_createtable(l, 0, 1);
	ScriptSrv->ExportCFunction("IsDialogueActive", CDlgSysem_IsDialogueActive);
	lua_setglobal(l, "DlgMgr");

	OK;
}
//---------------------------------------------------------------------

}