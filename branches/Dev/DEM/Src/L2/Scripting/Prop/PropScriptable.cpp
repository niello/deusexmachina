#include "PropScriptable.h"

#include <Scripting/ScriptServer.h>
#include <Game/Entity.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(ScriptClass);
	DefineString(Script);
};

BEGIN_ATTRS_REGISTRATION(PropScriptable)
	RegisterString(ScriptClass, ReadOnly);
	RegisterString(Script, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropScriptable, Game::CProperty);
ImplementFactory(Properties::CPropScriptable);
ImplementPropertyStorage(CPropScriptable, 256);
RegisterProperty(CPropScriptable);

void CPropScriptable::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::ScriptClass);
	Attrs.Append(Attr::Script);
}
//---------------------------------------------------------------------

void CPropScriptable::Activate()
{
	Game::CProperty::Activate();
	
	const nString& LuaClass = GetEntity()->Get<nString>(Attr::ScriptClass);

	//???to OnLoad?

	Obj = n_new(CEntityScriptObject(*GetEntity(), GetEntity()->GetUID().CStr(), "Entities"));
	n_assert(Obj->Init(LuaClass.IsValid() ? LuaClass.Get() : "CEntityScriptObject"));

	const nString& ScriptFile = GetEntity()->Get<nString>(Attr::Script);
	if (ScriptFile.IsValid()) Obj->LoadScriptFile("scripts:" + ScriptFile + ".lua");

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropScriptable, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(OnLoad, CPropScriptable, OnLoad);
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropScriptable, OnSave);
	PROP_SUBSCRIBE_PEVENT(OnDelete, CPropScriptable, OnDelete);
}
//---------------------------------------------------------------------

void CPropScriptable::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnLoad);
	UNSUBSCRIBE_EVENT(OnSave);
	UNSUBSCRIBE_EVENT(OnDelete);
	
	Obj->RunFunction("OnPropTerm");
	Obj = NULL;
	
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropScriptable::OnPropsActivated(const CEventBase& Event)
{
	if (ScriptSrv->BeginMixin(Obj.get_unsafe()))
	{
		GetEntity()->FireEvent(CStrID("ExposeSI"));
		ScriptSrv->EndMixin();
		Obj->RunFunction("OnPropInit");
	}
	else n_printf("Entity \"%s\": error when mixing-in script object\n", GetEntity()->GetUID().CStr());
	OK;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnLoad(const CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	Obj->LoadFields(pDB);
	OK;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnSave(const CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	Obj->SaveFields(pDB);
	OK;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnDelete(const CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	Obj->ClearFields(pDB);
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties