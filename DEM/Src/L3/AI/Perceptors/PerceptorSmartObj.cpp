#include "PerceptorSmartObj.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClassNoFactory(AI::CPerceptorIAO, AI::CPerceptor);
__ImplementClass(AI::CPerceptorIAO);

using namespace Properties;

void CPerceptorIAO::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	//???special IAO stimulus, may be one per action or with action list?
	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		Game::PEntity Ent = EntityMgr->GetEntity(pStimulus->SourceEntityID);
		CPropSmartObject* pSO = Ent->GetProperty<CPropSmartObject>();

		if (!pSO) return;

		CMemFactSmartObj Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactSmartObj pFact = (CMemFactSmartObj*)pActor->GetMemSystem().FindFact(Pattern);
		if (!pFact.IsValid())
		{
			pFact = (CMemFactSmartObj*)pActor->GetMemSystem().AddFact(CMemFactSmartObj::RTTI);
			pFact->pSourceStimulus = pStimulus;
			pFact->TypeID = pSO->GetTypeID();
			pActor->RequestGoalUpdate();
		}

		//!!!CALC!
		pFact->Confidence = Confidence;

		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.005f;
	}
}
//---------------------------------------------------------------------

} //namespace AI