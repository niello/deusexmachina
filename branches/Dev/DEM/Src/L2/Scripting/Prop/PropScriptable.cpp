#include "PropScriptable.h"

#include <Scripting/ScriptServer.h>
#include <Game/Entity.h>

//BEGIN_ATTRS_REGISTRATION(PropScriptable)
//	RegisterString(ScriptClass, ReadOnly);
//	RegisterString(Script, ReadOnly);
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropScriptable, 'PSCR', Game::CProperty);
__ImplementPropertyStorage(CPropScriptable);

void CPropScriptable::Activate()
{
	Game::CProperty::Activate();

	nString LuaClass;
	if (!GetEntity()->GetAttr<nString>(LuaClass, CStrID("ScriptClass")))
		LuaClass = "CEntityScriptObject";
	n_assert(LuaClass.IsValid());

	Obj = n_new(CEntityScriptObject(*GetEntity(), GetEntity()->GetUID().CStr(), "Entities"));
	n_assert(Obj->Init(LuaClass.CStr()));

	nString ScriptFile;
	if (GetEntity()->GetAttr<nString>(ScriptFile, CStrID("Script")) && ScriptFile.IsValid())
		Obj->LoadScriptFile("scripts:" + ScriptFile + ".lua");

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropScriptable, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(OnLoad, CPropScriptable, OnLoad);
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropScriptable, OnSave);
}
//---------------------------------------------------------------------

void CPropScriptable::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnLoad);
	UNSUBSCRIBE_EVENT(OnSave);
	
	Obj->RunFunction("OnPropTerm");
	Obj = NULL;
	
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropScriptable::OnPropsActivated(const Events::CEventBase& Event)
{
	if (ScriptSrv->BeginMixin(Obj.GetUnsafe()))
	{
		GetEntity()->FireEvent(CStrID("ExposeSI"));
		ScriptSrv->EndMixin();
		Obj->RunFunction("OnPropInit");
	}
	else n_printf("Entity \"%s\": error when mixing-in script object\n", GetEntity()->GetUID().CStr());
	OK;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnLoad(const Events::CEventBase& Event)
{
	//DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	//Obj->LoadFields(pDB);
	OK;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnSave(const Events::CEventBase& Event)
{
	//DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	//Obj->SaveFields(pDB);
	OK;
}
//---------------------------------------------------------------------

} // namespace Prop