#include "ValidatorDlgRuns.h"

#include <Story/Dlg/DlgSystem.h>

namespace AI
{
ImplementRTTI(AI::CValidatorDlgRuns, AI::CValidator);
ImplementFactory(AI::CValidatorDlgRuns);

bool CValidatorDlgRuns::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	return DlgSys->IsDialogueActive();
}
//---------------------------------------------------------------------

} //namespace AI