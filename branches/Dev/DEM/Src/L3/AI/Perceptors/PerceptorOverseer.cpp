#include "PerceptorOverseer.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Memory/MemFactOverseer.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?
#include <Data/DataArray.h>

namespace AI
{
ImplementRTTI(AI::CPerceptorOverseer, AI::CPerceptor);
ImplementFactory(AI::CPerceptorOverseer);

void CPerceptorOverseer::Init(const Data::CParams& Desc)
{
	//CPerceptor::Init(Desc);
	
	PDataArray Array = Desc.Get<PDataArray>(CStrID("Overseers"), NULL);
	
	//!!!need CStrID in PS!
	//if (Array.isvalid()) Array->FillArray(Overseers);
	
	if (Array.isvalid())
		for (CDataArray::iterator It = Array->Begin(); It != Array->End(); It++)
			Overseers.Append(CStrID((*It).GetValue<nString>().Get()));
}
//---------------------------------------------------------------------

void CPerceptorOverseer::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	if (Overseers.Size() < 1) return; // Only if no rule to dynamically detect overseers

	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		int i;
		for (i = 0; i < Overseers.Size(); ++i)
			if (pStimulus->SourceEntityID == Overseers[i]) break;

		if (i == Overseers.Size()) return;

		CMemFactOverseer Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactOverseer pFact = (CMemFactOverseer*)pActor->GetMemSystem().FindFact(Pattern);
		if (!pFact.isvalid())
		{
			pFact = (CMemFactOverseer*)pActor->GetMemSystem().AddFact(CMemFactOverseer::RTTI);
			pFact->pSourceStimulus = pStimulus;
			pActor->RequestGoalUpdate();
		}

		//!!!Must depend on distance & does overseer look actor!
		pFact->Confidence = Confidence;

		pFact->LastPerceptionTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.02f;
	}
}
//---------------------------------------------------------------------

} //namespace AI