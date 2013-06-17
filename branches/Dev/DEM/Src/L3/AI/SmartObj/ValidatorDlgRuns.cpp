#include "ValidatorDlgRuns.h"

#include <Dlg/DialogueManager.h>

namespace AI
{
__ImplementClass(AI::CValidatorDlgRuns, 'VDLG', AI::CValidator);

bool CValidatorDlgRuns::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	return DlgMgr->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI