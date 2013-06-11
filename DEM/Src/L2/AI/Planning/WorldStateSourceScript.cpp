#include "WorldStateSourceScript.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Scripting/PropScriptable.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClass(AI::CWorldStateSourceScript, 'WSSS', AI::CWorldStateSource);

void CWorldStateSourceScript::Init(Data::PParams Desc)
{
	n_assert(Desc.IsValid());
	Func = Desc->Get<nString>(CStrID("Func")).CStr();
}
//---------------------------------------------------------------------

//???to WorldState.h?
static EWSProp GetPropKeyByName(LPCSTR KeyName)
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

bool CWorldStateSourceScript::FillWorldState(const CActor* pActor, const CPropSmartObject* pSO, CWorldState& WS)
{
	if (!Func.IsValid()) OK;

	CPropScriptable* pScriptable = pSO->GetEntity()->GetProperty<CPropScriptable>();
	CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
	if (!pScriptObj) OK;

	CData RetVal;
	if (pScriptObj->RunFunctionOneArg(Func, pActor->GetEntity()->GetUID(), &RetVal) != Error)
	{
		if (RetVal.IsA<bool>()) return (bool)RetVal;
		if (RetVal.IsA<PParams>())
		{
			PParams Ret = RetVal;
			for (int i = 0; i < Ret->GetCount(); ++i)
			{
				CParam& Prm = Ret->Get(i);
				EWSProp Key = GetPropKeyByName(Prm.GetName().CStr());
				if (Key != WSP_Invalid)
				{
					// Conversion from nString to CStrID may be wrong, fix if so
					if (Prm.IsA<nString>())
					{
						LPCSTR Str = Prm.GetValue<nString>().CStr();
						EWSProp Value = GetPropKeyByName(Str);
						WS.SetProp(Key, (Value != WSP_Invalid) ? CData(Value) : CData(CStrID(Str)));
					}
					else WS.SetProp(Key, Prm.GetRawValue());
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

} //namespace AI