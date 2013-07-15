#include "ActionGoto.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionGoto, 'AGTO', AI::CAction);

//bool CActionGoto::Activate(CActor* pActor)
//{
//	// Derived classes can setup destination here before calling base method:
//	// pActor->GetNavSystem().SetDestPoint(...);
//	OK;
//}
////---------------------------------------------------------------------

EExecStatus CActionGoto::Update(CActor* pActor)
{
	switch (pActor->NavStatus)
	{
		case AINav_Invalid:		//???Autocreate sub-action to restore validity? Now Fail.
		case AINav_Failed:		return Failure;
		case AINav_Done:		return Success;
		case AINav_DestSet:		return Running;
		case AINav_Planning:
		{
			// Can follow temporary path
			return Running;
		}
		case AINav_Following:
		{
			// Derived classes can update target if necessary (if it is dynamic)
			// pActor->GetNavSystem().SetDestPoint(...);
			return AdvancePath(pActor);
		}
		default: n_error("CActionGoto::Update(): Unexpected navigation status '%d'", pActor->NavStatus);
	}

	return Failure;
}
//---------------------------------------------------------------------

void CActionGoto::Deactivate(CActor* pActor)
{
	if (SubAction.IsValid())
	{
		SubAction->Deactivate(pActor);
		SubAction = NULL;
	}

	if (pActor->NavStatus != AINav_Done &&
		pActor->NavStatus != AINav_Failed &&
		pActor->NavStatus != AINav_Invalid)
		pActor->GetNavSystem().Reset();
}
//---------------------------------------------------------------------

bool CActionGoto::IsValid(CActor* pActor) const
{
	return pActor->NavStatus != AINav_Invalid && pActor->NavStatus != AINav_Failed;
}
//---------------------------------------------------------------------

EExecStatus CActionGoto::AdvancePath(CActor* pActor)
{
	bool OffMesh = pActor->GetNavSystem().IsTraversingOffMesh();
	if (!pActor->GetNavSystem().GetPathEdges(Path, OffMesh ? 1 : 2)) return Failure;

	if (Path.GetCount() < 1) return Running;

	CStrID NewActionID = Path[0].Action;
	n_assert(NewActionID.IsValid());

	if (NewActionID != SubActionID)
	{
		if (SubAction.IsValid()) SubAction->Deactivate(pActor);

		SubActionID = NewActionID;

		static const CString ActNameBase("AI::CAction");
		SubAction = (CActionTraversePathEdge*)Factory->Create(ActNameBase + SubActionID.CStr());
		if (!SubAction->Activate(pActor))				
		{
			SubAction = NULL;
			pActor->GetNavSystem().Reset();
			return Failure;
		}
	}

	SubAction->UpdatePathEdge(pActor, &Path[0], (Path.GetCount() > 1) ? &Path[1] : NULL);
	EExecStatus Result = SubAction->Update(pActor);
	if (Result != Running)
	{
		SubAction->Deactivate(pActor);
		SubAction = NULL;
		SubActionID = CStrID::Empty;
		if (Result == Success)
		{
			pActor->GetNavSystem().EndEdgeTraversal();
			return Running; // NavSystem will tell us if we should finish
		}
		else
		{
			pActor->GetNavSystem().Reset();
			return Failure;
		}
	}

	return Running;
}
//---------------------------------------------------------------------

} //namespace AI