#include "PerceptorSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CPerceptorSmartObj, 'PESO', AI::CPerceptor);

using namespace Prop;

void CPerceptorSmartObj::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	//???special SO stimulus, may be one per action or with action list?
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
			pFact = pActor->GetMemSystem().AddFact<CMemFactSmartObj>();
			pFact->pSourceStimulus = pStimulus;
			pFact->TypeID = pSO->GetTypeID();
			pActor->RequestBehaviourUpdate();
		}

		//!!!CALC!
		pFact->Confidence = Confidence;

		pFact->LastPerceptioCTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.005f;
	}
}
//---------------------------------------------------------------------

} //namespace AI