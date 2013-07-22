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
	n_assert(pSO);
	return
		pActor &&
		pActor->GetEntity() != pSO->GetEntity() &&
		pSO->GetEntity()->GetProperty<CPropTalking>() &&
		!DlgMgr->IsDialogueActive();
}
//---------------------------------------------------------------------

}