#include "MemSystem.h"

#include <AI/PropActorBrain.h>

namespace AI
{

void CMemSystem::Update()
{
	float Now = 0.f;//(float)GameSrv->GetTime();

	CArray<CSensor*> ValidationSensors;

	// Here we validate facts that reside in memory but weren't updated by the last
	// sensor activity session. Facts will be accepted or rejected by sensors where
	// it is possible. Some sort of prediction can be applied to remaining facts.

	for (UPTR i = 0; i < Facts.GetListCount(); ++i)
	{
		//!!!can avoid second dictionary lookup, index will always be i!
		//???!!!cache sensor lists?!
		CArray<PSensor>::CIterator It = pActor->GetSensors().Begin();
		for (; It != pActor->GetSensors().End(); ++It)
			if ((*It)->ValidatesFactType(*Facts.GetKeyAt(i)))
				ValidationSensors.Add((*It));

		for (CMemFactNode ItCurr = Facts.GetHeadAt(i); ItCurr; /* empty */)
		{
			CMemFact* pFact = ItCurr->Get();

			if (pFact->LastUpdateTime < Now)
			{
				UPTR Result = Running;

				CArray<CSensor*>::CIterator It = ValidationSensors.Begin();
				for (; It != ValidationSensors.End(); ++It)
				{
					// Since we validate only facts not updated this frame, we definitely know here,
					// that sensor didn't sense stimulus that produced this fact
					Result = (*It)->ValidateFact(pActor, *pFact);
					if (Result != Running) break;
				}

				if (Result != Failure)
					pFact->Confidence -= (Now - pFact->LastUpdateTime) * pFact->ForgettingFactor;
				
				if (Result == Failure || pFact->Confidence <= 0.f)
				{
					CMemFactNode ItRemove = ItCurr;
					++ItCurr;
					Facts.Remove(ItRemove);
					continue;
				}

				// update facts here (can predict positions etc)

				pFact->LastUpdateTime = Now;
			}

			++ItCurr;
		}

		ValidationSensors.Clear();

		//???sort by confidence? may be only some fact types
	}
}
//---------------------------------------------------------------------

CMemFact* CMemSystem::FindFact(const CMemFact& Pattern, Data::CFlags FieldMask)
{
	CMemFactNode ItCurr = Facts.GetHead(Pattern.GetKey());

	while (ItCurr)
	{
		// Can lookup pointer to virtual function Match() once since facts are grouped by the class
		if ((*ItCurr)->Match(Pattern, FieldMask)) return *ItCurr;
		++ItCurr;
	}

	return nullptr;
}
//---------------------------------------------------------------------

}
