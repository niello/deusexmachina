#include "MemSystem.h"

//#include <AI/AIServer.h>
#include <AI/Prop/PropActorBrain.h>
#include <Game/GameServer.h> //???separate time source for AI?

namespace AI
{

/*
void CMemSystem::Init()
{
	for (int i = 0; i < Facts.GetListCount(); ++i)
	{
		nArray<PSensor>::iterator It = pActor->GetSensors().Begin();
		for (; It != pActor->GetSensors().End(); It++)
	}
}
//---------------------------------------------------------------------
*/

void CMemSystem::Update()
{
	float Now = (float)GameSrv->GetTime();

	nArray<CSensor*> ValidationSensors;

	for (int i = 0; i < Facts.GetListCount(); ++i)
	{
		//!!!can avoid second dictionary lookup, index will always be i!
		//???!!!cache sensor lists?!
		nArray<PSensor>::iterator It = pActor->GetSensors().Begin();
		for (; It != pActor->GetSensors().End(); It++)
			if ((*It)->ValidatesFactType(*Facts.GetKeyAt(i)))
				ValidationSensors.Append((*It));

		for (CMemFactNode* pCurr = Facts.GetHeadAt(i); pCurr; )
		{
			if (pCurr->Object->LastUpdateTime < Now)
			{
				EExecStatus Result = Running;

				nArray<CSensor*>::iterator It = ValidationSensors.Begin();
				for (; It != ValidationSensors.End(); It++)
				{
					// Since we validate only facts not updated this frame, we definitely know here,
					// that sensor didn't sense stimulus that produced this fact
					Result = (*It)->ValidateFact(pActor, *pCurr->Object);
					if (Result != Running) break;
				}

				if (Result != Failure)
					pCurr->Object->Confidence -= (Now - pCurr->Object->LastUpdateTime) * pCurr->Object->ForgettingFactor;
				
				if (Result == Failure || pCurr->Object->Confidence <= 0.f)
				{
					CMemFactNode* pRem = pCurr;
					pCurr = pCurr->GetSucc();
					Facts.Remove(pRem);
					continue;
				}

				// update facts here (can predict positions etc)

				pCurr->Object->LastUpdateTime = Now;
			}

			pCurr = pCurr->GetSucc();
		}

		ValidationSensors.Clear();

		//???sort by confidence? may be only some fact types
	}
}
//---------------------------------------------------------------------

CMemFact* CMemSystem::AddFact(const Core::CRTTI& Type)
{
	// remember list count
	CMemFactNode* pNode = Facts.Add((CMemFact*)CoreFct->Create(Type));
	// if list count changed, build validation sensor list for Type
	return pNode->Object;
}
//---------------------------------------------------------------------

CMemFact* CMemSystem::FindFact(const CMemFact& Pattern, CFlags FieldMask)
{
	CMemFactNode* pCurr = Facts.GetHead(Pattern.GetKey());

	while (pCurr)
	{
		// Can lookup pointer to virtual function Match() once since facts are grouped by the class
		if (pCurr->Object->Match(Pattern, FieldMask)) return pCurr->Object;
		pCurr = pCurr->GetSucc();
	}

	return NULL;
}
//---------------------------------------------------------------------

} //namespace AI