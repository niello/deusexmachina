#include "GoalWork.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Memory/MemFactOverseer.h>
#include <Game/Mgr/EntityManager.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
ImplementRTTI(AI::CGoalWork, AI::CGoal);
ImplementFactory(AI::CGoalWork);

using namespace Properties;

void CGoalWork::Init(PParams Params)
{
	WorkActionMap.Add(CStrID::Empty, CStrID("Work")); // Default, may move to params too

	PParams Map = Params->Get<PParams>(CStrID("ActionMap"), NULL);
	if (Map.isvalid())
		for (int i = 0; i < Map->GetCount(); ++i)
		{
			const CParam& Prm = Map->Get(i);
			WorkActionMap.Add(Prm.GetName(), CStrID(Prm.GetValue<nString>().Get()));
		}

	CGoal::Init(Params);
}
//---------------------------------------------------------------------

void CGoalWork::EvalRelevance(CActor* pActor)
{
	//???if working, keep relevance?

	IAO = CStrID::Empty;
	Action = CStrID::Empty;

	CMemFactSmartObj* pBest = NULL;
	Relevance = 0.f;
	
	CMemFactNode* pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactSmartObj::RTTI);
	for (; pCurr; pCurr = pCurr->GetSucc())
	{
		CMemFactSmartObj* pSOFact = (CMemFactSmartObj*)pCurr->Object.get();
		int Idx = WorkActionMap.FindIndex(pSOFact->TypeID);
		if (Idx != INVALID_INDEX)
		{
			Game::PEntity Ent = EntityMgr->GetEntityByID(pSOFact->pSourceStimulus->SourceEntityID);
			CPropSmartObject* pSO = Ent->FindProperty<CPropSmartObject>();
			n_assert(pSO);

			//!!!check not only HasAction, but also is action available for this actor in this SO state now!

			CStrID CurrAction = WorkActionMap.ValueAtIndex(Idx);
			if (pSOFact->Confidence > Relevance && pSO->HasAction(CurrAction))
			{
				pBest = pSOFact;
				Relevance = pSOFact->Confidence;
				Action = CurrAction;
			}
		}
	}

	if (!pBest || Relevance == 0.f) return;

	float TotalOverseersConf = 0.f;
	
	pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactOverseer::RTTI);
	for (; pCurr; pCurr = pCurr->GetSucc())
		TotalOverseersConf += ((CMemFactOverseer*)pCurr->Object.get())->Confidence;

	IAO = pBest->pSourceStimulus->SourceEntityID;
	Relevance *= (1.f + TotalOverseersConf) * PersonalityFactor;
}
//---------------------------------------------------------------------

void CGoalWork::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_UsingSmartObj, IAO);
	Dest.SetProp(WSP_Action, Action);
}
//---------------------------------------------------------------------

} //namespace AI