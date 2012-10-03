#include "PropTalking.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/Mgr/EntityManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Properties
{
using namespace Scripting;

int CPropTalking_SayPhrase(lua_State* l)
{
	//args: EntityScriptObject's this table, Phrase ID
	SETUP_ENT_SI_ARGS(2);

	Game::CEntity* pTalkingEnt = This->GetEntity();
	CPropTalking* pTalking = pTalkingEnt ? pTalkingEnt->FindProperty<CPropTalking>() : NULL;

	if (pTalking)
	{
		CStrID PhraseID = CStrID(lua_tostring(l, 2));
		if (PhraseID.IsValid()) pTalking->SayPhrase(PhraseID);
	}

	return 0;
}
//---------------------------------------------------------------------

bool CPropTalking::ExposeSI(const CEventBase& Event)
{
	ScriptSrv->ExportCFunction("SayPhrase", CPropTalking_SayPhrase);
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
