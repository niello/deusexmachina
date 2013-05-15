#include "ValidatorDlgRuns.h"

#include <Story/Dlg/DialogueManager.h>

namespace AI
{
__ImplementClassNoFactory(AI::CValidatorDlgRuns, AI::CValidator);
__ImplementClass(AI::CValidatorDlgRuns);

bool CValidatorDlgRuns::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	return DlgMgr->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI