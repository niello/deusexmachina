#include "ActionWander.h"

#include <Game/GameServer.h>
#include <AI/Movement/Actions/ActionGoto.h>
#include <AI/Movement/Actions/ActionFace.h>
#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionWander, 'AWDR', AI::CAction);

//void CActionWander::Init()
//{
//}
////---------------------------------------------------------------------

bool CActionWander::SelectAction(CActor* pActor)
{
	if (CurrAction.IsValid()) CurrAction->Deactivate(pActor);

	//!!!can tune all these probabilities and timings!
	float Rnd = n_rand();
	if (Rnd < 0.6f)
	{
		// Can also start from current direction & limit angle
		vector2 Dest(0.f, -1.f);
		Dest.rotate(n_rand(-PI, PI));
		if (Rnd < 0.3f)
		{
			// NavSystem automatically clamps a destination to the navmesh
			Dest *= (n_rand() * 11.5f + 3.5f);
			pActor->GetNavSystem().SetDestPoint(vector3(Dest.x + InitialPos.x, pActor->Position.y, Dest.y + InitialPos.y));
			CurrAction = n_new(CActionGoto);		
			//vector3 Loc;
			//if (pActor->GetNavSystem().GetRandomValidLocation(15.f, Loc))
			//{
			//	pActor->GetNavSystem().SetDestPoint(Loc);
			//	CurrAction = n_new(CActionGoto);
			//}
		}
		else
		{
			pActor->GetMotorSystem().SetFaceDirection(vector3(Dest.x, 0.f, Dest.y));
			CurrAction = n_new(CActionFace);
		}
	}
	else
	{
		CurrAction = NULL;
		NextActSelectioCTime = (float)GameSrv->GetTime() + n_rand() * 5.f + 5.f;
		OK;
	}

	return CurrAction->Activate(pActor);
}
//---------------------------------------------------------------------

bool CActionWander::Activate(CActor* pActor)
{
	InitialPos.set(pActor->Position.x, pActor->Position.z);
	return SelectAction(pActor);
}
//---------------------------------------------------------------------

DWORD CActionWander::Update(CActor* pActor)
{
	if ((CurrAction.IsValid() && CurrAction->Update(pActor) == Running) ||
		NextActSelectioCTime > (float)GameSrv->GetTime())
	{
		return Running;
	}

	return SelectAction(pActor) ? Running : Failure;
}
//---------------------------------------------------------------------

void CActionWander::Deactivate(CActor* pActor)
{
	if (CurrAction.IsValid())
	{
		CurrAction->Deactivate(pActor);
		CurrAction = NULL;
	}
}
//---------------------------------------------------------------------

} //namespace AI