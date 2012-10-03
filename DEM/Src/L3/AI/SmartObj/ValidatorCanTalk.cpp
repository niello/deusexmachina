#include "ValidatorCanTalk.h"

#include <Story/Dlg/DlgSystem.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <Chr/Prop/PropTalking.h>

namespace AI
{
ImplementRTTI(AI::CValidatorCanTalk, AI::CValidator);
ImplementFactory(AI::CValidatorCanTalk);

bool CValidatorCanTalk::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	n_assert(pActor && pSO);
	CPropTalking* pPropTalking = pSO->GetEntity()->FindProperty<CPropTalking>();
	return	pActor->GetEntity() != pSO->GetEntity() &&
			pPropTalking && /*pPropTalking->GetDialogue() &&*/ !DlgSys->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI