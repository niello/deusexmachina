#include "WorldStateSourceScript.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Scripting/PropScriptable.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CWorldStateSourceScript, 'WSSS', AI::CWorldStateSource);

void CWorldStateSourceScript::Init(Data::PParams Desc)
{
	n_assert(Desc.IsValidPtr());
	Func = Desc->Get<CString>(CStrID("Func")).CStr();
}
//---------------------------------------------------------------------

//???to WorldState.h?
static EWSProp GetPropKeyByName(const char* KeyName)
{
	if (!strcmp(KeyName, "WSP_AtEntityPos")) return WSP_AtEntityPos;
	if (!strcmp(KeyName, "WSP_UsingSmartObj")) return WSP_UsingSmartObj;
	if (!strcmp(KeyName, "WSP_TargetIsDead")) return WSP_TargetIsDead;
	if (!strcmp(KeyName, "WSP_Action")) return WSP_Action;
	if (!strcmp(KeyName, "WSP_HasItem")) return WSP_HasItem;
	if (!strcmp(KeyName, "WSP_ItemEquipped")) return WSP_ItemEquipped;
	return WSP_Invalid;
}
//---------------------------------------------------------------------

bool CWorldStateSourceScript::FillWorldState(const CActor* pActor, const Prop::CPropSmartObject* pSO, CWorldState& WS)
{
	if (!Func.IsValid()) OK;

	Prop::CPropScriptable* pScriptable = pSO->GetEntity()->GetProperty<Prop::CPropScriptable>();
	Prop::CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : nullptr;
	if (!pScriptObj) OK;

	Data::CData RetVal;
	if (!ExecResultIsError(pScriptObj->RunFunctionOneArg(Func, pActor->GetEntity()->GetUID(), &RetVal)))
	{
		if (RetVal.IsA<bool>()) return (bool)RetVal;
		if (RetVal.IsA<Data::PParams>())
		{
			Data::PParams Ret = RetVal;
			for (UPTR i = 0; i < Ret->GetCount(); ++i)
			{
				Data::CParam& Prm = Ret->Get(i);
				EWSProp Key = GetPropKeyByName(Prm.GetName().CStr());
				if (Key != WSP_Invalid)
				{
					// Conversion from CString to CStrID may be wrong, fix if so
					if (Prm.IsA<CString>())
					{
						const char* Str = Prm.GetValue<CString>().CStr();
						EWSProp Value = GetPropKeyByName(Str);
						WS.SetProp(Key, (Value != WSP_Invalid) ? Data::CData(Value) : Data::CData(CStrID(Str)));
					}
					else WS.SetProp(Key, Prm.GetRawValue());
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

}