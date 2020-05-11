#include "ActionWander.h"

#include <Game/GameServer.h>
//#include <AI/Movement/Actions/ActionGoto.h>
//#include <AI/Movement/Actions/ActionFace.h>
#include <AI/PropActorBrain.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionWander, 'AWDR', AI::CAction);

bool CActionWander::SelectAction(CActor* pActor)
{
	if (CurrAction.IsValidPtr()) CurrAction->Deactivate(pActor);

	//!!!can tune all these probabilities and timings!
	float Rnd = Math::RandomFloat();
	if (Rnd < 0.6f)
	{
		// Can also start from current direction & limit angle
		vector2 Dest(0.f, -1.f);
		Dest.rotate(Math::RandomFloat(-PI, PI));
		if (Rnd < 0.3f)
		{
			// NavSystem automatically clamps a destination to the navmesh
			Dest *= (Math::RandomFloat() * 11.5f + 3.5f);
			pActor->GetNavSystem().SetDestPoint(vector3(Dest.x + InitialPos.x, pActor->Position.y, Dest.y + InitialPos.y));
			//CurrAction = n_new(CActionGoto);		
			//vector3 Loc;
			//if (pActor->GetNavSystem().GetRandomValidLocation(15.f, Loc))
			//{
			//	pActor->GetNavSystem().SetDestPoint(Loc);
			//	CurrAction = n_new(CActionGoto);
			//}
		}
		else
		{
			//pActor->GetMotorSystem().SetFaceDirection(vector3(Dest.x, 0.f, Dest.y));
			//CurrAction = n_new(CActionFace);
		}
	}
	else
	{
		CurrAction = nullptr;
		NextActSelectioCTime = (float)GameSrv->GetTime() + Math::RandomFloat() * 5.f + 5.f;
		OK;
	}

	FAIL;
	//return CurrAction->Activate(pActor);
}
//---------------------------------------------------------------------

bool CActionWander::Activate(CActor* pActor)
{
	InitialPos.set(pActor->Position.x, pActor->Position.z);
	return SelectAction(pActor);
}
//---------------------------------------------------------------------

UPTR CActionWander::Update(CActor* pActor)
{
	if ((CurrAction.IsValidPtr() && CurrAction->Update(pActor) == Running) ||
		NextActSelectioCTime > (float)GameSrv->GetTime())
	{
		return Running;
	}

	return SelectAction(pActor) ? Running : Failure;
}
//---------------------------------------------------------------------

void CActionWander::Deactivate(CActor* pActor)
{
	if (CurrAction.IsValidPtr())
	{
		CurrAction->Deactivate(pActor);
		CurrAction = nullptr;
	}
}
//---------------------------------------------------------------------

} //namespace AI
