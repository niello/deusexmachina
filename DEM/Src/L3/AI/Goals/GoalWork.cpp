#include "GoalWork.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Memory/MemFactOverseer.h>
#include <Game/EntityManager.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClass(AI::CGoalWork, 'GWRK', AI::CGoal);

using namespace Prop;

void CGoalWork::Init(PParams Params)
{
	WorkActionMap.Add(CStrID::Empty, CStrID("Work")); // Default, may move to params too

	PParams Map = Params->Get<PParams>(CStrID("ActionMap"), NULL);
	if (Map.IsValid())
		for (int i = 0; i < Map->GetCount(); ++i)
		{
			const CParam& Prm = Map->Get(i);
			WorkActionMap.Add(Prm.GetName(), CStrID(Prm.GetValue<CString>().CStr()));
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
	
	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactSmartObj::RTTI);
	for (; It; ++It)
	{
		CMemFactSmartObj* pSOFact = (CMemFactSmartObj*)It->Get();
		int Idx = WorkActionMap.FindIndex(pSOFact->TypeID);
		if (Idx != INVALID_INDEX)
		{
			Game::PEntity Ent = EntityMgr->GetEntity(pSOFact->pSourceStimulus->SourceEntityID);
			CPropSmartObject* pSO = Ent->GetProperty<CPropSmartObject>();
			n_assert(pSO);

			//!!!check not only HasAction, but also is action available for this actor in this SO state now!

			CStrID CurrAction = WorkActionMap.ValueAt(Idx);
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
	
	It = pActor->GetMemSystem().GetFactsByType(CMemFactOverseer::RTTI);
	for (; It; ++It)
		TotalOverseersConf += ((CMemFactOverseer*)It->Get())->Confidence;

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