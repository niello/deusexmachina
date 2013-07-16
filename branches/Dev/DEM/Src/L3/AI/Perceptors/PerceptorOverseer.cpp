#include "PerceptorOverseer.h"

#include <AI/PropActorBrain.h>
#include <AI/Memory/MemFactOverseer.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <Game/GameServer.h> //???separate time source for AI?
#include <Data/DataArray.h>

namespace AI
{
__ImplementClass(AI::CPerceptorOverseer, 'PEOV', AI::CPerceptor);

void CPerceptorOverseer::Init(const Data::CParams& Desc)
{
	//CPerceptor::Init(Desc);
	
	PDataArray Array = Desc.Get<PDataArray>(CStrID("Overseers"), NULL);
	
	//!!!need CStrID in PS!
	//if (Array.IsValid()) Array->FillArray(Overseers);
	
	if (Array.IsValid())
		for (CDataArray::CIterator It = Array->Begin(); It != Array->End(); It++)
			Overseers.Add(CStrID((*It).GetValue<CString>().CStr()));
}
//---------------------------------------------------------------------

void CPerceptorOverseer::ProcessStimulus(CActor* pActor, CStimulus* pStimulus, float Confidence)
{
	if (Overseers.GetCount() < 1) return; // Only if no rule to dynamically detect overseers

	if (pStimulus->IsA(CStimulusVisible::RTTI))
	{
		int i;
		for (i = 0; i < Overseers.GetCount(); ++i)
			if (pStimulus->SourceEntityID == Overseers[i]) break;

		if (i == Overseers.GetCount()) return;

		CMemFactOverseer Pattern;
		Pattern.pSourceStimulus = pStimulus;
		PMemFactOverseer pFact = (CMemFactOverseer*)pActor->GetMemSystem().FindFact(Pattern);
		if (!pFact.IsValid())
		{
			pFact = pActor->GetMemSystem().AddFact<CMemFactOverseer>();
			pFact->pSourceStimulus = pStimulus;
			pActor->RequestGoalUpdate();
		}

		//!!!Must depend on distance & does overseer look actor!
		pFact->Confidence = Confidence;

		pFact->LastPerceptioCTime =
		pFact->LastUpdateTime = (float)GameSrv->GetTime();
		pFact->ForgettingFactor = 0.02f;
	}
}
//---------------------------------------------------------------------

} //namespace AI