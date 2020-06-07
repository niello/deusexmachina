#include "PerceptorSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CPerceptorSmartObj, 'PESO', AI::CPerceptor);

void CPerceptorSmartObj::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	//???special SO stimulus, may be one per action or with action list?
	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		//Game::PEntity Ent = GameSrv->GetEntityMgr()->GetEntity(pStimulus->SourceEntityID);
		//Prop::CPropSmartObject* pSO = Ent->GetProperty<Prop::CPropSmartObject>();

		//if (!pSO) return;
		return;

		CMemFactSmartObj Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactSmartObj pFact = (CMemFactSmartObj*)pActor->GetMemSystem().FindFact(Pattern);
		if (pFact.IsNullPtr())
		{
			pFact = pActor->GetMemSystem().AddFact<CMemFactSmartObj>();
			pFact->pSourceStimulus = pStimulus;
			//pFact->TypeID = pSO->GetTypeID();
			pActor->RequestBehaviourUpdate();
		}

		//!!!CALC!
		pFact->Confidence = Confidence;

		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = 0.f;//(float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.005f;
	}
}
//---------------------------------------------------------------------

}
