#include "PropSceneNode.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Scripting/PropScriptable.h>
#include <Game/Entity.h>
#include <Scene/Events/SetTransform.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{
using namespace Scripting;

int CPropSceneNode_SetPosition(lua_State* l)
{
	//args: EntityScriptObject's this table, x, y, z
	SETUP_ENT_SI_ARGS(4);

	//???allow to change attrs by ref? through non-const &
	matrix44 Tfm = This->GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	Tfm.m[3][0] = (float)lua_tonumber(l, 2);
	Tfm.m[3][1] = (float)lua_tonumber(l, 3);
	Tfm.m[3][2] = (float)lua_tonumber(l, 4);
	This->GetEntity()->FireEvent(Event::SetTransform(Tfm));
	return 0;
}
//---------------------------------------------------------------------

int CPropSceneNode_GetPosition(lua_State* l)
{
	//args: EntityScriptObject's this table
	//ret:  x, y, z
	SETUP_ENT_SI_ARGS(1);

	const vector3& Pos = This->GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();
	lua_pushnumber(l, Pos.x);
	lua_pushnumber(l, Pos.y);
	lua_pushnumber(l, Pos.z);
	return 3;
}
//---------------------------------------------------------------------

void CPropSceneNode::EnableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ExportCFunction("SetPosition", CPropSceneNode_SetPosition);
	ScriptSrv->ExportCFunction("GetPosition", CPropSceneNode_GetPosition);
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

void CPropSceneNode::DisableSI(CPropScriptable& Prop)
{
	n_verify_dbg(ScriptSrv->BeginMixin(Prop.GetScriptObject().GetUnsafe()));
	ScriptSrv->ClearField("AddItem");
	ScriptSrv->ClearField("RemoveItem");
	ScriptSrv->ClearField("HasItem");
	ScriptSrv->EndMixin();
}
//---------------------------------------------------------------------

}