#pragma once
#include <AI/Planning/Goal.h>
#include <Data/Dictionary.h>

// This goal makes actor want to work, if he knows about some SO offering work and
// if he is aware of an overseer.

namespace AI
{

class CGoalWork: public CGoal
{
	FACTORY_CLASS_DECL;

protected:

	// SO type ID to action ID
	CDict<CStrID, CStrID> WorkActionMap;

	CStrID SO;
	CStrID Action;

public:

	virtual void Init(Data::PParams Params);
	virtual void EvalRelevance(CActor* pActor);
	virtual void GetDesiredProps(CWorldState& Dest);
};

}
