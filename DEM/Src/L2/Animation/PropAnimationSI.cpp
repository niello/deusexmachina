#include "PropAnimation.h"

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

int CPropAnimation_GetAnimLength(lua_State* l)
{
	//args: EntityScriptObject's this table, Clip ID
	//ret: float length in seconds
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	lua_pushnumber(l, pProp ? (lua_Number)pProp->GetAnimLength(CStrID(lua_tostring(l, 2))) : 0.f);
	return 1;
}
//---------------------------------------------------------------------

int CPropAnimation_StartAnim(lua_State* l)
{
	//args: EntityScriptObject's this table, Clip ID, [see below]
	//ret: int task ID
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	if (!pProp) return 0;

	CStrID ClipID = CStrID(lua_tostring(l, 2));
	bool Loop = (ArgCount > 2) ? !!lua_toboolean(l, 3) : false;
	float Offset = (ArgCount > 3) ? (float)lua_tonumber(l, 4) : 0.f;
	float Speed = (ArgCount > 4) ? (float)lua_tonumber(l, 5) : 1.f;
	bool Manual = (ArgCount > 5) ? !!lua_toboolean(l, 6) : false;
	DWORD Priority = (ArgCount > 6) ? lua_tointeger(l, 7) : AnimPriority_Default;
	float Weight = (ArgCount > 7) ? (float)lua_tonumber(l, 8) : 1.f;
	float FadeInTime = (ArgCount > 8) ? (float)lua_tonumber(l, 9) : 0.f;
	float FadeOutTime = (ArgCount > 9) ? (float)lua_tonumber(l, 10) : 0.f;

	int TaskID = pProp->StartAnim(ClipID, Loop, Offset, Speed, Manual, Priority, Weight, FadeInTime, FadeOutTime);
	lua_pushinteger(l, (lua_Integer)TaskID);
	return 1;
}
//---------------------------------------------------------------------

int CPropAnimation_PauseAnim(lua_State* l)
{
	//args: EntityScriptObject's this table, Task ID, [bool Pause]
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	bool Pause = (ArgCount > 2) ? !!lua_toboolean(l, 3) : true;
	if (pProp) pProp->PauseAnim(lua_tointeger(l, 2), Pause);
	return 0;
}
//---------------------------------------------------------------------

int CPropAnimation_StopAnim(lua_State* l)
{
	//args: EntityScriptObject's this table, Task ID, [fadeout time]
	SETUP_ENT_SI_ARGS(2);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	if (!pProp) return 0;

	float FadeOutTime = (ArgCount > 2) ? (float)lua_tonumber(l, 3) : -1.f;

	pProp->StopAnim(lua_tointeger(l, 2), FadeOutTime);
	return 0;
}
//---------------------------------------------------------------------

int CPropAnimation_SetAnimCursorPos(lua_State* l)
{
	//args: EntityScriptObject's this table, Task ID, animation cursor pos
	SETUP_ENT_SI_ARGS(3);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	if (!pProp) return 0;
	pProp->SetAnimCursorPos(lua_tointeger(l, 2), (float)lua_tonumber(l, 3));
	return 0;
}
//---------------------------------------------------------------------

int CPropAnimation_SetPose(lua_State* l)
{
	//args: EntityScriptObject's this table, Clip ID, Time, [WrapTime]
	//ret: bool success
	SETUP_ENT_SI_ARGS(3);
	CPropAnimation* pProp = This->GetEntity()->GetProperty<CPropAnimation>();
	if (pProp)
	{
		bool WrapTime = (ArgCount > 3) ? !!lua_toboolean(l, 4) : false;
		bool Result = pProp->SetPose(CStrID(lua_tostring(l, 2)), (float)lua_tonumber(l, 3), WrapTime);
		lua_pushboolean(l, Result ? 1 : 0);
	}
	else lua_pushboolean(l, 0);
	return 1;
}
//---------------------------------------------------------------------

void CPropAnimation::EnableSI(CPropScriptable& Prop)
{
	n_assert_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("GetAnimLength", CPropAnimation_GetAnimLength);
	ScriptSrv->ExportCFunction("StartAnim", CPropAnimation_StartAnim);
	ScriptSrv->ExportCFunction("PauseAnim", CPropAnimation_PauseAnim);
	ScriptSrv->ExportCFunction("StopAnim", CPropAnimation_StopAnim);
	ScriptSrv->ExportCFunction("SetAnimCursorPos", CPropAnimation_SetAnimCursorPos);
	ScriptSrv->ExportCFunction("SetPose", CPropAnimation_SetPose);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropAnimation::DisableSI(CPropScriptable& Prop)
{
	n_assert_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("GetAnimLength");
	ScriptSrv->ClearField("StartAnim");
	ScriptSrv->ClearField("PauseAnim");
	ScriptSrv->ClearField("StopAnim");
	ScriptSrv->ClearField("SetAnimCursorPos");
	ScriptSrv->ClearField("SetPose");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}