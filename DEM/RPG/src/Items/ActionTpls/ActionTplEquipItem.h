#pragma once
#include <AI/Planning/ActionTpl.h>

// EquipItem action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionTplEquipItem: public CActionTpl
{
	FACTORY_CLASS_DECL;

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplEquipItem> PActionTplEquipItem;

}
