#pragma once
#include <AI/Planning/ActionTpl.h>

// Template of GotoSmartObj action, that performs custom actions on interactive objects.

namespace AI
{

class CActionTplGotoSmartObj: public CActionTpl //!!!???CActionTplGoto?!
{
	FACTORY_CLASS_DECL;

private:

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplGotoSmartObj> PActionTplGotoSmartObj;

}
