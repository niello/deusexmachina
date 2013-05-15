#pragma once
#ifndef __DEM_L2_AI_ACTION_TPL_GOTO_SO_H__
#define __DEM_L2_AI_ACTION_TPL_GOTO_SO_H__

#include <AI/Planning/ActionTpl.h>

// Template of GotoSmartObj action, that performs custom actions on interactive objects.

namespace AI
{

class CActionTplGotoSmartObj: public CActionTpl //!!!???CActionTplGoto?!
{
	__DeclareClass(CActionTplGotoSmartObj);

private:

public:

	virtual void		Init(PParams Params);
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

__RegisterClassInFactory(CActionTplGotoSmartObj);

typedef Ptr<CActionTplGotoSmartObj> PActionTplGotoSmartObj;

}

#endif