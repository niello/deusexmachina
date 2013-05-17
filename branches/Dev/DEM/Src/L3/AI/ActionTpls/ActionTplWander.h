#pragma once
#ifndef __DEM_L3_AI_ACTION_TPL_WANDER_H__
#define __DEM_L3_AI_ACTION_TPL_WANDER_H__

#include <AI/Planning/ActionTpl.h>

// Wander action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionTplWander: public CActionTpl
{
	__DeclareClass(CActionTplWander);

public:

	virtual void		Init(PParams Params);
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplWander> PActionTplWander;

}

#endif