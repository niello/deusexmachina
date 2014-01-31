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

public:

	vector3			Point;
	EMovementType	MvmtType;

	virtual bool	IsAvailableTo(const CActor* pActor);
	virtual PAction	BuildPlan();
	virtual bool	OnPlanSet(CActor* pActor);
};

typedef Ptr<CTaskGoto> PTaskGoto;

}

#endif