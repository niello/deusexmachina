#include "GoalWork.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <AI/Memory/MemFactOverseer.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CGoalWork, 'GWRK', AI::CGoal);

using namespace Prop;

void CGoalWork::Init(Data::PParams Params)
{
	WorkActionMap.Add(CStrID::Empty, CStrID("Work")); // Default, may move to params too

	Data::PParams Map = Params->Get<Data::PParams>(CStrID("ActionMap"), NULL);
	if (Map.IsValidPtr())
		for (UPTR i = 0; i < Map->GetCount(); ++i)
		{
			const Data::CParam& Prm = Map->Get(i);
			WorkActionMap.Add(Prm.GetName(), CStrID(Prm.GetValue<CString>().CStr()));
		}

	CGoal::Init(Params);
}
//---------------------------------------------------------------------

void CGoalWork::EvalRelevance(CActor* pActor)
{
	//???if working, keep relevance?

	SO = CStrID::Empty;
	Action = CStrID::Empty;

	CMemFactSmartObj* pBest = NULL;
	Relevance = 0.f;

	// Remember any known smart object to work on

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

	// Increase relevance if overseers watch me

	float TotalOverseersConf = 0.f;

	It = pActor->GetMemSystem().GetFactsByType(CMemFactOverseer::RTTI);
	for (; It; ++It)
		TotalOverseersConf += It->Get()->Confidence;

	SO = pBest->pSourceStimulus->SourceEntityID;
	Relevance *= (1.f + TotalOverseersConf) * PersonalityFactor;
}
//---------------------------------------------------------------------

void CGoalWork::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_UsingSmartObj, SO);
	Dest.SetProp(WSP_Action, Action);
}
//---------------------------------------------------------------------

}