#include "ValidatorScript.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <Scripting/Prop/PropScriptable.h>

namespace AI
{
ImplementRTTI(AI::CValidatorScript, AI::CValidator);
ImplementFactory(AI::CValidatorScript);

using namespace Data;

void CValidatorScript::Init(PParams Desc)
{
	n_assert(Desc.isvalid());
	ConditionFunc = Desc->Get<nString>(CStrID("ConditionFunc")).Get();
	//???!!!relevance func?!
}
//---------------------------------------------------------------------

bool CValidatorScript::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	if (ConditionFunc.IsValid())
	{
		CPropScriptable* pScriptable = pSO->GetEntity()->FindProperty<CPropScriptable>();
		CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
		if (pScriptObj && pScriptObj->RunFunctionData(ConditionFunc, pActor->GetEntity()->GetUniqueID()) != Success) FAIL;
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