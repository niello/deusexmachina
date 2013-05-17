#pragma once
#ifndef __DEM_L2_AI_GOAL_IDLE_H__
#define __DEM_L2_AI_GOAL_IDLE_H__

#include <AI/Planning/Goal.h>

// Special goal that makes actor want nothing but to be idle. Use this to ensure that actor
// has a Goal object even when he has no goal. Application can provide idle action, that
// will setup animation or other specific implementation of idle behaviour.

namespace AI
{

class CGoalIdle: public CGoal
{
	__DeclareClass(CGoalIdle);

public:

	virtual void EvalRelevance(CActor* pActor) { Relevance = PersonalityFactor * 0.01f; }
	virtual void GetDesiredProps(CWorldState& Dest);
};

}

#endif