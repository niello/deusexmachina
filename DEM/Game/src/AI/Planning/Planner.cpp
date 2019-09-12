#include "Planner.h"

#include <AI/Behaviour/ActionSequence.h>
#include <AI/PropActorBrain.h>
#include <Core/Factory.h>

const CString StrActTplPrefix("AI::CActionTpl");

namespace AI
{
static CArray<CActionTpl*> ActionsAdded;

using namespace Data;

void CPlanner::RegisterActionTpl(const char* Name, Data::PParams Params)
{
	if (!FindActionTpl(Name))
	{
		PActionTpl NewTpl = (CActionTpl*)Factory->Create(StrActTplPrefix + Name);
		NewTpl->Init(Params);
		ActionTpls.Add(NewTpl); //???dictionary? CStrID -> Tpl?
	}
}
//---------------------------------------------------------------------

void CPlanner::EndActionTpls()
{
	for (; NewActIdx < ActionTpls.GetCount(); ++NewActIdx)
	{
		const CWorldState& Effects = ActionTpls[NewActIdx]->GetEffects();

		for (UPTR i = 0; i < WSP_Count; ++i)
			if (Effects.IsPropSet((EWSProp)i))
				EffectToActions[i].Add(ActionTpls[NewActIdx]);
	}

	ActionsAdded.Reallocate(ActionTpls.GetCount(), 0);
}
//---------------------------------------------------------------------

// Determine the current world state values for world state properties.
void CPlanner::MergeWorldStates(CWorldState& WSCurr, const CWorldState& WSGoal, const CWorldState& WSActor)
{
	for (UPTR i = 0; i < WSP_Count; ++i)
		if (WSGoal.IsPropSet((EWSProp)i) && !WSCurr.IsPropSet((EWSProp)i))
			WSCurr.SetPropFrom((EWSProp)i, WSActor);
}
//---------------------------------------------------------------------

bool CPlanner::IsPlanValid(CActor* pActor, CNode* pNode, const CWorldState& WSActor)
{
	CActionTpl* pAction = pNode->pAction;
	if (!pAction) FAIL;

	CWorldState WorldState;
	MergeWorldStates(WorldState, pNode->WSCurr, WSActor);

	CNode* pNodeParent = nullptr;
	
	while (pAction)
	{
		pNodeParent = pNode->pParent;

		// Validate world state effects
		// Action is valid is there are effects not met in the current world state
		bool Valid = false;
		for (UPTR i = 0; i < WSP_Count; ++i)
		{
			const CData& Effect = pAction->GetEffects().GetProp((EWSProp)i);

			if (Effect.IsValid())
				if (Effect.IsA<EWSProp>())
				{
					EWSProp PcVal = Effect.GetValue<EWSProp>();
					if (!WorldState.IsPropSet((EWSProp)i) ||
						!pNodeParent->WSGoal.IsPropSet(PcVal) ||
						WorldState.GetProp((EWSProp)i) != pNodeParent->WSGoal.GetProp(PcVal))
					{
						Valid = true;
						break;
					}
				}
				else
				{
					if (!WorldState.IsPropSet((EWSProp)i) ||
						WorldState.GetProp((EWSProp)i) != Effect)
					{
						Valid = true;
						break;
					}
				}
		}

		if (!Valid) FAIL;

		// Validate world state preconditions
		if (pNode->HasPreconditions)
			for (UPTR i = 0; i < WSP_Count; ++i)
			{
				const CData& Precondition = pNode->WSPreconditions.GetProp((EWSProp)i);

				if (Precondition.IsValid())
					if (Precondition.IsA<EWSProp>())
					{
						EWSProp PcVal = Precondition.GetValue<EWSProp>();
						if (WorldState.IsPropSet((EWSProp)i) &&
							pNodeParent->WSGoal.IsPropSet(PcVal) &&
							WorldState.GetProp((EWSProp)i) != pNodeParent->WSGoal.GetProp(PcVal))
							FAIL;
					}
					else
					{
						if (WorldState.IsPropSet((EWSProp)i) &&
							WorldState.GetProp((EWSProp)i) != Precondition)
							FAIL;
					}
			}

		// Checked in FindNeighbors, result can't change in synchronous environment
		//if (!pAction->ValidateContextPreconditions(pActor, pNodeParent->WSGoal)) FAIL;

		//!!!Probability was checked here!

		// Apply world state effects
		for (UPTR i = 0; i < WSP_Count; ++i)
		{
			const CData& Effect = pAction->GetEffects().GetProp((EWSProp)i);

			if (Effect.IsValid())
				if (Effect.IsA<EWSProp>())
				{
					EWSProp PcVal = Effect.GetValue<EWSProp>();
					if (pNodeParent->WSGoal.IsPropSet(PcVal))
						WorldState.SetProp((EWSProp)i, pNodeParent->WSGoal.GetProp(PcVal));
				}
				else WorldState.SetProp((EWSProp)i, Effect);
		}

		pNode = pNodeParent;
		pAction = pNode->pAction;
	}

	// Check does the WorldState satisfy the goal world state
	for (UPTR i = 0; i < WSP_Count; ++i)
		if (pNodeParent->WSGoal.IsPropSet((EWSProp)i) &&
			(!WorldState.IsPropSet((EWSProp)i) ||
			pNodeParent->WSGoal.GetProp((EWSProp)i) != WorldState.GetProp((EWSProp)i)))
				FAIL;

	OK;
}
//---------------------------------------------------------------------

int CPlanner::CmpPlannerNodes(const void* First, const void* Second)
{
	if (!First || !Second) return 0;
	return (*(const CNode**)Second)->pAction->GetPrecedence() - (*(const CNode**)First)->pAction->GetPrecedence();
}
//---------------------------------------------------------------------

// A neighbor is an Action that has an effect potentially satisfying one of the goal props.
// Neighbors are based only on the property key, not on the associated value.
void CPlanner::FillNeighbors(CActor* pActor, const CNode& Node, CArray<CNode*>& OutNeighbors)
{
	OutNeighbors.Clear();
	ActionsAdded.Clear();

	for (UPTR i = 0; i < WSP_Count; ++i)
	{
		// Neighbor satisfies property if it is set in both Curr & Goal WS and Curr value != Goal value
		if (!Node.WSCurr.IsPropSet((EWSProp)i) ||
			!Node.WSGoal.IsPropSet((EWSProp)i) ||
			Node.WSCurr.GetProp((EWSProp)i) == Node.WSGoal.GetProp((EWSProp)i))
			continue;

		CArray<CActionTpl*>& Actions = EffectToActions[i];
		for (CArray<CActionTpl*>::CIterator ppAction = Actions.Begin(); ppAction != Actions.End(); ++ppAction)
			if (ActionsAdded.FindIndexSorted(*ppAction) == INVALID_INDEX &&
				pActor->IsActionAvailable(*ppAction) &&
				(*ppAction)->ValidateContextPreconditions(pActor, Node.WSGoal))
			{					
				CNode* pNewNode = NodePool.Construct();
				pNewNode->pAction = *ppAction;
				OutNeighbors.Add(pNewNode);
				ActionsAdded.InsertSorted(*ppAction);
				if (OutNeighbors.GetCount() >= ActionTpls.GetCount()) break;
			}
	}

	// Sort actions by precedence, so the plan will be sorted by precedence too
	if (OutNeighbors.GetCount() > 1)
		qsort((void*)OutNeighbors.Begin(), (size_t)OutNeighbors.GetCount(), sizeof(CNode*), CmpPlannerNodes);
}
//---------------------------------------------------------------------

PAction CPlanner::BuildPlan(CActor* pActor, CGoal* pGoal)
{
	n_assert(pActor && pGoal);

	CWorldState WSActor;
	pActor->FillWorldState(WSActor);

	// Some sources recommend to use IDA* instead of basic A*.
	// We use A* at least for now.

	Data::CList<CNode*> OpenList, ClosedList;
	Data::CList<CNode*>::CIterator ItCurrNode = OpenList.AddFront(NodePool.Construct());

	CNode* pCurrNode = *ItCurrNode;

	pGoal->GetDesiredProps(pCurrNode->WSGoal);
	MergeWorldStates(pCurrNode->WSCurr, pCurrNode->WSGoal, WSActor);

	pCurrNode->Goal = 0;
	pCurrNode->Fitness = pCurrNode->WSCurr.GetDiffCount(pCurrNode->WSGoal);

	CArray<CNode*> Neighbors;

	while (true)
	{
		//!!!PERF
		// Optimization: "How to Achieve Lightning-Fast A*",
		// AI Game Programming Wisdom, p. 133.
		// Specifically "Be a Cheapskate" on p. 140.

		ItCurrNode = OpenList.Begin();

		if (!ItCurrNode) break; // No valid plan exists

		for (Data::CList<CNode*>::CIterator It = ItCurrNode; It; ++It)
			if ((*It)->Fitness < (*ItCurrNode)->Fitness)
				ItCurrNode = It;

		OpenList.Remove(ItCurrNode, &pCurrNode);
		ClosedList.AddFront(pCurrNode);

		//!!!this re-checks plan from the beginning! can re-check only part changed since last check?
		if (IsPlanValid(pActor, pCurrNode, WSActor)) break; // Valid plan found

		FillNeighbors(pActor, *pCurrNode, Neighbors);

		for (UPTR i = 0; i < Neighbors.GetCount(); ++i)
		{
			CNode* pNeighbor = Neighbors[i];

			pNeighbor->WSCurr = pCurrNode->WSCurr;
			pNeighbor->WSGoal = pCurrNode->WSGoal;
			pNeighbor->HasPreconditions =
				pNeighbor->pAction->GetPreconditions(pActor, pNeighbor->WSPreconditions, pCurrNode->WSGoal);

			// Apply effects from goal. Solve properties of the world state that need to 
			// be satisfied that match the action's effects
			for (UPTR i = 0; i < WSP_Count; ++i)
			{
				const CData& Effect = pNeighbor->pAction->GetEffects().GetProp((EWSProp)i);

				if (Effect.IsValid())
				{
					const CData& Result = pNeighbor->WSGoal.GetProp((Effect.IsA<EWSProp>()) ? Effect.GetValue<EWSProp>() : (EWSProp)i);
					if (Result.IsValid())
					{
						n_assert2(!Result.IsA<EWSProp>(), "Setting WS prop to variable not allowed here!");
						pNeighbor->WSCurr.SetProp((EWSProp)i, Result);
					}
				}
			}

			// Apply preconditions
			if (pNeighbor->HasPreconditions)
			{
				for (UPTR i = 0; i < WSP_Count; ++i)
				{
					const CData& Precondition = pNeighbor->WSPreconditions.GetProp((EWSProp)i);
					if (!Precondition.IsValid()) continue;
					if (Precondition.IsA<EWSProp>())
					{
						EWSProp PcVal = Precondition.GetValue<EWSProp>();
						if (pNeighbor->WSGoal.IsPropSet(PcVal))
							pNeighbor->WSGoal.SetProp((EWSProp)i, pNeighbor->WSGoal.GetProp(PcVal));
					}
					else pNeighbor->WSGoal.SetProp((EWSProp)i, Precondition);
				}
			}

			MergeWorldStates(pNeighbor->WSCurr, pNeighbor->WSGoal, WSActor);

			pNeighbor->Goal = pCurrNode->Goal + pNeighbor->pAction->GetCost();
			pNeighbor->Fitness = pNeighbor->Goal + pNeighbor->WSCurr.GetDiffCount(pNeighbor->WSGoal);
			pNeighbor->pParent = pCurrNode;

			n_assert(pNeighbor->Fitness < I32_MAX);
			//if (pNeighbor->Fitness == I32_MAX) NodePool.Destroy(pNeighbor);

			OpenList.AddFront(pNeighbor);
		}

		// No need now, but can optimize:
		// Reorder the list OPEN in order of increasing Fitness values. (Ties among minimal Fitness values 
		// are resolved in favor of the deepest node in the search tree).
	}

	PAction Plan;
	PActionSequence Seq;

	DBG_ONLY(Sys::Log("Planner -> '%s' Begin plan\n", pActor->GetEntity()->GetUID()));

	while (pCurrNode && pCurrNode->pAction)
	{
		PAction CurrAction = pCurrNode->pAction->CreateInstance(pCurrNode->pParent->WSGoal);

		if (CurrAction.IsValidPtr())
		{
#ifdef _DEBUG
			CString DbgString;
			CurrAction->GetDebugString(DbgString);
			Sys::Log("Planner -> '%s' Action added: '%s'\n", pActor->GetEntity()->GetUID(), DbgString.CStr());
#endif

			if (Plan.IsNullPtr()) Plan = CurrAction;
			else
			{
				if (Seq.IsNullPtr())
				{
					Seq = n_new(CActionSequence);
					Seq->AddChild(Plan);
					Plan = Seq;
				}
				Seq->AddChild(CurrAction);
			}
		}

		pCurrNode = pCurrNode->pParent;
	}

	DBG_ONLY(Sys::Log("Planner -> '%s' End plan\n", pActor->GetEntity()->GetUID()));

	while (OpenList.RemoveBack(&pCurrNode)) NodePool.Destroy(pCurrNode);
	while (ClosedList.RemoveBack(&pCurrNode)) NodePool.Destroy(pCurrNode);

	return Plan;
}
//---------------------------------------------------------------------

}