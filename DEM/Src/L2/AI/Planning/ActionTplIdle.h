#pragma once
#ifndef __DEM_L2_AI_ACTION_TPL_IDLE_H__
#define __DEM_L2_AI_ACTION_TPL_IDLE_H__

#include <AI/Planning/ActionTpl.h>

// Idle action does exactly nothing and never ends. This is coupled with GoalIdle to provide
// 'no goal, no behavoiur' state for decision making enabled actors. User can derive its own
// Idle action to activate some Idle animation on actors or to switch its state.

namespace AI
{

class CActionTplIdle: public CActionTpl
{
	__DeclareClass(CActionTplIdle);

public:

	virtual void		Init(Data::PParams Params);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

typedef Ptr<CActionTplIdle> PActionTplIdle;

}

#endif