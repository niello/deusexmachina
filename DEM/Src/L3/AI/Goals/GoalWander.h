#pragma once
#ifndef __DEM_L3_AI_GOAL_WANDER_H__
#define __DEM_L3_AI_GOAL_WANDER_H__

#include <AI/Planning/Goal.h>

// This goal makes actor want to wander. See CActionWander.

namespace AI
{

class CGoalWander: public CGoal
{
	DeclareRTTI;
	DeclareFactory(CGoalWander);

public:

	virtual void EvalRelevance(CActor* pActor) { Relevance = PersonalityFactor; }
	virtual void GetDesiredProps(CWorldState& Dest);
};

RegisterFactory(CGoalWander);

}

#endif