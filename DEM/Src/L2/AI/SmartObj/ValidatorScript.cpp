#include "ValidatorScript.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Scripting/PropScriptable.h>

namespace AI
{
__ImplementClass(AI::CValidatorScript, 'VLSC', AI::CValidator);

using namespace Data;

void CValidatorScript::Init(PParams Desc)
{
	n_assert(Desc.IsValid());
	ConditionFunc = Desc->Get<nString>(CStrID("ConditionFunc")).CStr();
	//???!!!relevance func?!
}
//---------------------------------------------------------------------

bool CValidatorScript::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	if (ConditionFunc.IsValid())
	{
		CPropScriptable* pScriptable = pSO->GetEntity()->GetProperty<CPropScriptable>();
		CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
		if (pScriptObj && pScriptObj->RunFunctionOneArg(ConditionFunc, pActor->GetEntity()->GetUID()) != Success) FAIL;
	}
	OK;
}
//---------------------------------------------------------------------

float CValidatorScript::GetRelevance(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	n_error("IMPLEMENT ME!");
	return 0.f;
}
//---------------------------------------------------------------------

} //namespace AI