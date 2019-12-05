#pragma once
#include <AI/Behaviour/Action.h>

// Idle action does exactly nothing and never ends. This is coupled with GoalIdle to provide
// 'no goal, no behavoiur' state for decision making enabled actors. User can implement its own
// Idle action to activate some Idle animation on actors or to switch its state. It can be
// derived from this or completely new.

namespace AI
{

class CActionIdle: public CAction
{
	FACTORY_CLASS_DECL;

public:

	virtual UPTR Update(CActor* pActor) { return Running; }
};

typedef Ptr<CActionIdle> PActionIdle;

}
