#include "PerceptorObstacle.h"

#include <AI/PropActorBrain.h>
#include <AI/Movement/Memory/MemFactObstacle.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CPerceptorObstacle, 'PEOB', AI::CPerceptor);

void CPerceptorObstacle::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		//!!!can filter stimuli by distance, ignoring too far obstacles!
		if (pStimulus->Radius < 0.01f) return;

		CMemFactObstacle Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactObstacle pFact = (CMemFactObstacle*)pActor->GetMemSystem().FindFact(Pattern);
		if (pFact.IsNullPtr())
		{
			pFact = pActor->GetMemSystem().AddFact<CMemFactObstacle>();
			pFact->pSourceStimulus = pStimulus;

			////???update even if fact found? only if R & H of obstacle can change
			pFact->Radius = pStimulus->Radius;
			//???pFact->Height = pStimulus->Height;
		}
		
		pFact->Confidence = Confidence; //???prioritize by distance to actor?
		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = 0.f;//(float)GameSrv->GetTime(); //???or AI frame number?
		pFact->ForgettingFactor = 0.05f; //???!!!different rates for static & moving?!
		pFact->Position = pStimulus->Position;
	}
}
//---------------------------------------------------------------------

} //namespace AI
