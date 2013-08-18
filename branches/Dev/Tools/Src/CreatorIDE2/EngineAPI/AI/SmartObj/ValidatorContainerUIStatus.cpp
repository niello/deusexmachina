#include "ValidatorContainerUIStatus.h"

namespace AI
{
__ImplementClass(CValidatorContainerUIStatus, "VCUS", CValidator);

bool CValidatorContainerUIStatus::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	return false;
}
//---------------------------------------------------------------------

} //namespace AI