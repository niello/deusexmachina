#pragma once
#include <AI/Planning/ActionTpl.h>

// Template of UseSmartObj action, that performs custom actions on interactive objects.

namespace AI
{

class CActionTplUseSmartObj: public CActionTpl
{
	FACTORY_CLASS_DECL;

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplUseSmartObj> PActionTplUseSmartObj;

}
