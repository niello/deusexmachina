#include "PropAnimation.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/Entity.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Properties
{
using namespace Scripting;

int CPropAnimation_GetAnimLength(lua_State* l)
{
	//args: EntityScriptObject's this table, Clip ID
	//ret: float length in seconds
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->FindProperty<CPropAnimation>();
	lua_pushnumber(l, pProp ? (lua_Number)pProp->GetAnimLength(CStrID(lua_tostring(l, 2))) : 0.f);
	return 1;
}
//---------------------------------------------------------------------

int CPropAnimation_StartAnim(lua_State* l)
{
	//args: EntityScriptObject's this table, Clip ID, [Loop, ]
	//ret: int task ID
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->FindProperty<CPropAnimation>();
	if (!pProp) return 0;

	CStrID ClipID = CStrID(lua_tostring(l, 2));
	bool Loop = (ArgCount > 2) ? !!lua_toboolean(l, 3) : false;
	float Offset = (ArgCount > 3) ? (float)lua_tonumber(l, 4) : 0.f;
	float Speed = (ArgCount > 4) ? (float)lua_tonumber(l, 5) : 1.f;
	DWORD Priority = (ArgCount > 5) ? lua_tointeger(l, 6) : 0;
	float Weight = (ArgCount > 6) ? (float)lua_tonumber(l, 7) : 1.f;
	float FadeInTime = (ArgCount > 7) ? (float)lua_tonumber(l, 8) : 0.f;
	float FadeOutTime = (ArgCount > 8) ? (float)lua_tonumber(l, 9) : 0.f;

	int TaskID = pProp->StartAnim(ClipID, Loop, Offset, Speed, Priority, Weight, FadeInTime, FadeOutTime);
	lua_pushinteger(l, (lua_Integer)TaskID);
	return 1;
}
//---------------------------------------------------------------------

bool CPropAnimation::ExposeSI(const CEventBase& Event)
{
	ScriptSrv->ExportCFunction("GetAnimLength", CPropAnimation_GetAnimLength);
	ScriptSrv->ExportCFunction("StartAnim", CPropAnimation_StartAnim);
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
