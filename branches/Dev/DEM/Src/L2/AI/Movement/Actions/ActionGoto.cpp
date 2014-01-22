#include "ActionGoto.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionGoto, 'AGTO', AI::CAction);

DWORD CActionGoto::Update(CActor* pActor)
{
	switch (pActor->NavState)
	{
		case AINav_Failed:		return Failure;
		case AINav_Done:		return Success;
		case AINav_DestSet:		return Running;
		case AINav_Planning:
		case AINav_Following:	return AdvancePath(pActor);
		default: Core::Error("CActionGoto::Update(): Unexpected navigation status '%d'", pActor->NavState);
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

	if (!pActor->IsNavSystemIdle())
		pActor->GetNavSystem().Reset(pActor->IsAtPoint(pActor->GetNavSystem().GetDestPoint(), true));
}
//---------------------------------------------------------------------

bool CActionGoto::IsValid(CActor* pActor) const
{
	return	pActor->IsAtPoint(pActor->GetNavSystem().GetDestPoint(), true) ||
			pActor->NavState != AINav_Failed;
}
//---------------------------------------------------------------------

DWORD CActionGoto::AdvancePath(CActor* pActor)
{
	//???don't request edges every tick? Path remains valid until something happens!
	//???smth like NavSys->HasPathChanged()? set to false in GetPathEdges, to true when path changes
	bool OffMesh = pActor->GetNavSystem().IsTraversingOffMesh();
	DWORD EdgeCount;
	//???request one edge when recover to avoid smoothing?
	if (!pActor->GetNavSystem().GetPathEdges(Path, OffMesh ? 1 : 2, EdgeCount)) return Failure;
	if (!EdgeCount) return Running;

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
			pActor->GetNavSystem().Reset(false);
			return Failure;
		}
	}

	SubAction->UpdatePathEdge(pActor, &Path[0], (EdgeCount > 1 && NewActionID == Path[1].Action) ? &Path[1] : NULL);
	DWORD Result = SubAction->Update(pActor);
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
			pActor->GetNavSystem().Reset(false);
			return Failure;
		}
	}

	return Running;
}
//---------------------------------------------------------------------

}