#include "PropTalking.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <Game/EntityManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

int CPropTalking_SayPhrase(lua_State* l)
{
	//args: EntityScriptObject's this table, Phrase ID
	SETUP_ENT_SI_ARGS(2);

	Game::CEntity* pTalkingEnt = This->GetEntity();
	CPropTalking* pTalking = pTalkingEnt ? pTalkingEnt->GetProperty<CPropTalking>() : NULL;

	if (pTalking)
	{
		CStrID PhraseID = CStrID(lua_tostring(l, 2));
		if (PhraseID.IsValid()) pTalking->SayPhrase(PhraseID);
	}

	return 0;
}
//---------------------------------------------------------------------

void CPropTalking::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("SayPhrase", CPropTalking_SayPhrase);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropTalking::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("SayPhrase");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}