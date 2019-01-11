#include "PropTrigger.h"

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

int CPropTrigger_EnableTrigger(lua_State* l)
{
	//args: EntityScriptObject's this table
	SETUP_ENT_SI_ARGS(1);
	This->GetEntity()->GetProperty<CPropTrigger>()->SetEnabled(true);
	return 0;
}
//---------------------------------------------------------------------

int CPropTrigger_DisableTrigger(lua_State* l)
{
	//args: EntityScriptObject's this table
	SETUP_ENT_SI_ARGS(1);
	This->GetEntity()->GetProperty<CPropTrigger>()->SetEnabled(false);
	return 0;
}
//---------------------------------------------------------------------

void CPropTrigger::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ExportCFunction("EnableTrigger", CPropTrigger_EnableTrigger);
	ScriptSrv->ExportCFunction("DisableTrigger", CPropTrigger_DisableTrigger);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropTrigger::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().Get()));
	ScriptSrv->ClearField("EnableTrigger");
	ScriptSrv->ClearField("DisableTrigger");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}