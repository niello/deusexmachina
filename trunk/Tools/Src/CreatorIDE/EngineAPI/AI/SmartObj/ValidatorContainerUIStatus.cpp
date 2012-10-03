#include "ValidatorContainerUIStatus.h"

namespace AI
{
ImplementRTTI(AI::CValidatorContainerUIStatus, AI::CValidator);
ImplementFactory(AI::CValidatorContainerUIStatus);

bool CValidatorContainerUIStatus::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	return false;
}
//---------------------------------------------------------------------

} //namespace AI