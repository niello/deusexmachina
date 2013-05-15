#pragma once
#ifndef __DEM_L2_AI_ACTION_IDLE_H__
#define __DEM_L2_AI_ACTION_IDLE_H__

#include <AI/Behaviour/Action.h>

// Idle action does exactly nothing and never ends. This is coupled with GoalIdle to provide
// 'no goal, no behavoiur' state for decision making enabled actors. User can implement its own
// Idle action to activate some Idle animation on actors or to switch its state. It can be
// derived from this or completely new.

namespace AI
{

class CActionIdle: public CAction
{
	__DeclareClass(CActionIdle);

public:

	virtual EExecStatus	Update(CActor* pActor) { return Running; }
};

__RegisterClassInFactory(CActionIdle);

typedef Ptr<CActionIdle> PActionIdle;

}

#endif