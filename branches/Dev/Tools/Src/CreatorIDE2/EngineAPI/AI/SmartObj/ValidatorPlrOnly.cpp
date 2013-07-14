#include "ValidatorPlrOnly.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(CValidatorPlrOnly);

bool CValidatorPlrOnly::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	//!!!tmp solution, later use smth like PartyMgr!
	return pActor->GetEntity()->GetUniqueID() == "GG";
}
//---------------------------------------------------------------------

} //namespace AI