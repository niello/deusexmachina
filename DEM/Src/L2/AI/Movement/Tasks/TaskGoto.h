#pragma once
#ifndef __DEM_L2_AI_TASK_GOTO_H__
#define __DEM_L2_AI_TASK_GOTO_H__

#include <AI/Behaviour/Task.h>
#include <AI/Movement/MotorSystem.h> // For enum

// Basic goto command, sets Goto action as actor's plan

namespace AI
{

class CTaskGoto: public CTask
{
	__DeclareClass(CTaskGoto);

protected:

	//!!!store movement type, min & max reach dist, may be destination!

public:

	vector3			Point;
	float			MinDistance;
	float			MaxDistance;
	EMovementType	MvmtType;

	CTaskGoto(): MinDistance(0.f), MaxDistance(0.001f) {}

	virtual bool	IsAvailableTo(const CActor* pActor);
	virtual PAction	BuildPlan();
	virtual bool	OnPlanSet(CActor* pActor);
};

typedef Ptr<CTaskGoto> PTaskGoto;

}

#endif