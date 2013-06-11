#include "PropAIHints.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
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

bool CPropAIHints::ExposeSI(const Events::CEventBase& Event)
{
	ScriptSrv->ExportCFunction("EnableStimulus", CPropAIHints_EnableStimulus);
	OK;
}
//---------------------------------------------------------------------

} // namespace Prop