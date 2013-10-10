#pragma once
#ifndef __DEM_L3_AI_ACTION_TPL_EQUIP_ITEM_H__
#define __DEM_L3_AI_ACTION_TPL_EQUIP_ITEM_H__

#include <AI/Planning/ActionTpl.h>

// EquipItem action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionTplEquipItem: public CActionTpl
{
	__DeclareClass(CActionTplEquipItem);

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplEquipItem> PActionTplEquipItem;

}

#endif