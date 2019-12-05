#pragma once
#include <AI/Planning/Goal.h>

// Special goal that makes actor want nothing but to be idle. Use this to ensure that actor
// has a Goal object even when he has no goal. Application can provide idle action, that
// will setup animation or other specific implementation of idle behaviour.

namespace AI
{

class CGoalIdle: public CGoal
{
	FACTORY_CLASS_DECL;

public:

	virtual void EvalRelevance(CActor* pActor) { Relevance = PersonalityFactor * 0.01f; }
	virtual void GetDesiredProps(CWorldState& Dest);
};

}
