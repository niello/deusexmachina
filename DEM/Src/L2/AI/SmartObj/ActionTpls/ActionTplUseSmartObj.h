#pragma once
#ifndef __DEM_L2_AI_ACTION_TPL_USE_SO_H__
#define __DEM_L2_AI_ACTION_TPL_USE_SO_H__

#include <AI/Planning/ActionTpl.h>

// Template of UseSmartObj action, that performs custom actions on interactive objects.

namespace AI
{

class CActionTplUseSmartObj: public CActionTpl
{
	__DeclareClass(CActionTplUseSmartObj);

protected:

	bool				GetSOPreconditions(CActor* pActor, CWorldState& WS, CStrID SOEntityID, CStrID ActionID) const;

public:

	virtual void		Init(Data::PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplUseSmartObj> PActionTplUseSmartObj;

}

#endif