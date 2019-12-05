#pragma once
#include <AI/Planning/Goal.h>

// This goal makes actor want to wander. See CActionWander.
// Relevance base is always 1.f, so actor wants it more than anything.
// Maybe nonzero alert state should invalidate or decrease relevance of this goal.

namespace AI
{

class CGoalWander: public CGoal
{
	FACTORY_CLASS_DECL;

public:

	virtual void EvalRelevance(CActor* pActor) { Relevance = PersonalityFactor; }
	virtual void GetDesiredProps(CWorldState& Dest);
};

}
