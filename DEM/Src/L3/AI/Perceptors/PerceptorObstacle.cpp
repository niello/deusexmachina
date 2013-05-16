#include "PerceptorObstacle.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Movement/Memory/MemFactObstacle.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?

namespace AI
{
__ImplementClass(AI::CPerceptorObstacle, 'PEOB', AI::CPerceptor);

void CPerceptorObstacle::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		//!!!can filter stimuli by distance, ignoring too far obstacles!
		if (pStimulus->Radius < 0.01f) return;

		CMemFactObstacle Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactObstacle pFact = (CMemFactObstacle*)pActor->GetMemSystem().FindFact(Pattern);
		if (!pFact.IsValid())
		{
			pFact = pActor->GetMemSystem().AddFact<CMemFactObstacle>();
			pFact->pSourceStimulus = pStimulus;

			////???update even if fact found? only if R & H of obstacle can change
			pFact->Radius = pStimulus->Radius;
			//???pFact->Height = pStimulus->Height;
		}
		
		pFact->Confidence = Confidence; //???prioritize by distance to actor?
		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime(); //???or AI frame number?
		pFact->ForgettingFactor = 0.05f; //???!!!different rates for static & moving?!
		pFact->Position = pStimulus->Position;
	}
}
//---------------------------------------------------------------------

} //namespace AI