#include "ValidatorCanTalk.h"

#include <Story/Dlg/DialogueManager.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <Chr/Prop/PropTalking.h>

namespace AI
{
__ImplementClassNoFactory(AI::CValidatorCanTalk, AI::CValidator);
__ImplementClass(AI::CValidatorCanTalk);

bool CValidatorCanTalk::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	n_assert(pActor && pSO);
	CPropTalking* pPropTalking = pSO->GetEntity()->GetProperty<CPropTalking>();
	return	pActor->GetEntity() != pSO->GetEntity() &&
			pPropTalking && /*pPropTalking->GetDialogue() &&*/ !DlgMgr->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI