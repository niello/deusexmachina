#pragma once
#ifndef __DEM_L3_AI_GOAL_WORK_H__
#define __DEM_L3_AI_GOAL_WORK_H__

#include <AI/Planning/Goal.h>
#include <Data/Dictionary.h>

// This goal makes actor want to work, if he knows aboutsome IAO offering work and
// if he is aware of overseer.

namespace AI
{

class CGoalWork: public CGoal
{
	__DeclareClass(CGoalWork);

protected:

	// IAO type ID to action ID
	CDict<CStrID, CStrID> WorkActionMap;

	CStrID IAO;
	CStrID Action;

public:

	virtual void Init(PParams Params);
	virtual void EvalRelevance(CActor* pActor);
	virtual void GetDesiredProps(CWorldState& Dest);
};

}

#endif