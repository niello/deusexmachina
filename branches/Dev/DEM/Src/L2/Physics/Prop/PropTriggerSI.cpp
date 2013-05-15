#include "PropTrigger.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/Entity.h>
#include <Physics/Collision/Shape.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Properties
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

bool CPropTrigger::ExposeSI(const CEventBase& Event)
{
	ScriptSrv->ExportCFunction("EnableTrigger", CPropTrigger_EnableTrigger);
	ScriptSrv->ExportCFunction("DisableTrigger", CPropTrigger_DisableTrigger);
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties