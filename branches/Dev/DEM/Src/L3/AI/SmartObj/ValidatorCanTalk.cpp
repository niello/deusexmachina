#include "ValidatorCanTalk.h"

#include <Dlg/DialogueManager.h>
#include <Dlg/Prop/PropTalking.h>
#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>

namespace AI
{
__ImplementClass(AI::CValidatorCanTalk, 'VCTK', AI::CValidator);

bool CValidatorCanTalk::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	n_assert(pActor && pSO);
	CPropTalking* pPropTalking = pSO->GetEntity()->GetProperty<CPropTalking>();
	return	pActor->GetEntity() != pSO->GetEntity() &&
			pPropTalking && /*pPropTalking->GetDialogue() &&*/ !DlgMgr->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI