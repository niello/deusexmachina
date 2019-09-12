#include "PerceptorOverseer.h"

#include <AI/PropActorBrain.h>
#include <AI/Memory/MemFactOverseer.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CPerceptorOverseer, 'PEOV', AI::CPerceptor);

void CPerceptorOverseer::Init(const Data::CParams& Desc)
{
	//CPerceptor::Init(Desc);
	
	Data::PDataArray Array = Desc.Get<Data::PDataArray>(CStrID("Overseers"), nullptr);
	if (Array.IsValidPtr()) Array->FillArray(Overseers);
}
//---------------------------------------------------------------------

void CPerceptorOverseer::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	if (Overseers.GetCount() < 1) return; // Only if no rule to dynamically detect overseers

	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		UPTR i;
		for (i = 0; i < Overseers.GetCount(); ++i)
			if (pStimulus->SourceEntityID == Overseers[i]) break;

		if (i == Overseers.GetCount()) return;

		CMemFactOverseer Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactOverseer pFact = (CMemFactOverseer*)pActor->GetMemSystem().FindFact(Pattern);
		if (pFact.IsNullPtr())
		{
			pFact = pActor->GetMemSystem().AddFact<CMemFactOverseer>();
			pFact->pSourceStimulus = pStimulus;
			pActor->RequestBehaviourUpdate();
		}

		//!!!Must depend on distance & does overseer see actor or not!
		pFact->Confidence = Confidence;

		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.02f;
	}
}
//---------------------------------------------------------------------

}