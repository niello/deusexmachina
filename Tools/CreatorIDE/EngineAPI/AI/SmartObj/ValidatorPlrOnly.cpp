#include "ValidatorPlrOnly.h"

#include <AI/Prop/PropActorBrain.h>

namespace AI
{
ImplementRTTI(AI::CValidatorPlrOnly, AI::CValidator);
ImplementFactory(AI::CValidatorPlrOnly);

bool CValidatorPlrOnly::IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction)
{
	//!!!tmp solution, later use smth like PartyMgr!
	return pActor->GetEntity()->GetUniqueID() == "GG";
}
//---------------------------------------------------------------------

} //namespace AI