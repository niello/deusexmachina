#include "ActionSequence.h"

namespace AI
{

bool CActionSequence::Activate(CActor* pActor)
{
	n_assert(!ppCurrChild);
	if (Child.GetCount() < 1) FAIL;
	ppCurrChild = Child.Begin();
	return (*ppCurrChild)->Activate(pActor);
}
//---------------------------------------------------------------------

EExecStatus CActionSequence::Update(CActor* pActor)
{
	n_assert(ppCurrChild);

	while (true)
	{
		EExecStatus Result = (*ppCurrChild)->Update(pActor);

		if (Result == Running) return Running;

		(*ppCurrChild)->Deactivate(pActor);
		
		if (Result == Failure) return Failure;
		
		if (++ppCurrChild == Child.End())
		{
			ppCurrChild = NULL;
			return Success;
		}
		
		if (!(*ppCurrChild)->Activate(pActor)) return Failure;
	}
}
//---------------------------------------------------------------------

void CActionSequence::Deactivate(CActor* pActor)
{
	if (ppCurrChild) (*ppCurrChild)->Deactivate(pActor);
}
//---------------------------------------------------------------------

}