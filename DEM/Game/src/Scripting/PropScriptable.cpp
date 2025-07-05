#include "PropScriptable.h"

#include <Scripting/ScriptServer.h>
#include <Game/Entity.h>
#include <Core/Factory.h>

namespace Prop
{
FACTORY_CLASS_IMPL(Prop::CPropScriptable, 'PSCR', Game::CProperty);
__ImplementPropertyStorage(CPropScriptable);

CPropScriptable::CPropScriptable() {}
CPropScriptable::~CPropScriptable() {}

bool CPropScriptable::InternalActivate()
{
	CString LuaClass;
	if (!GetEntity()->GetAttr<CString>(LuaClass, CStrID("ScriptClass")))
		LuaClass = "CEntityScriptObject";
	n_assert(LuaClass.IsValid());

	Obj = n_new(CEntityScriptObject(*GetEntity(), GetEntity()->GetUID().CStr(), "Entities"));
	n_assert(Obj->Init(LuaClass.CStr()));

	CString ScriptFile;
	if (GetEntity()->GetAttr<CString>(ScriptFile, CStrID("Script")) && ScriptFile.IsValid())
		Obj->LoadScriptFile(("Scripts:" + ScriptFile + ".lua").CStr());

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropScriptable, OnPropsActivated);

	OK;
}
//---------------------------------------------------------------------

void CPropScriptable::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	
	Obj->RunFunction("OnPropTerm");
	Obj = nullptr;
}
//---------------------------------------------------------------------

bool CPropScriptable::OnPropsActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Obj->RunFunction("OnPropInit"); //???or use event subscription?
	OK;
}
//---------------------------------------------------------------------

}
