#include "PropAIHints.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <Game/Entity.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

int CPropAIHints_EnableStimulus(lua_State* l)
{
	//args: EntityScriptObject's this table, Stimulus ID, bool Enable
	SETUP_ENT_SI_ARGS(3);
	This->GetEntity()->GetProperty<CPropAIHints>()->EnableStimulus(CStrID(lua_tostring(l, 2)), lua_toboolean(l, 3) != 0);
	return 0;
}
//---------------------------------------------------------------------

void CPropAIHints::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("EnableStimulus", CPropAIHints_EnableStimulus);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropAIHints::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("EnableStimulus");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}