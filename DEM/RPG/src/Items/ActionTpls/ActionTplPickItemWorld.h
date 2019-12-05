#pragma once
#include <AI/Planning/ActionTpl.h>

// Actor picks item represented as entity in the location to satisfy WSP_HasItem.
// This tpl generates no action, it makes planner plan UseSmartObj with right params instead.

namespace AI
{

class CActionTplPickItemWorld: public CActionTpl
{
	FACTORY_CLASS_DECL;

private:

	//!!!only for synchronous env!
	CStrID ItemEntityID;

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplPickItemWorld> PActionTplPickItemWorld;

}
