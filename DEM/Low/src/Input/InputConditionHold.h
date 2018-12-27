#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_HOLD_H__
#define __DEM_L1_INPUT_CONDITION_HOLD_H__

#include <Input/InputCondition.h>

// Event condition that is triggered when Time seconds elapsed since the last
// ButtonDown of the corresponding button. ButtonUp stops and resets the timer.
// If repeat feature is enabled, timer will restart after triggering an event,
// and event will be triggered repeatedly.

namespace Input
{

class CInputConditionHold: public CInputConditionEvent
{
	__DeclareClass(CInputConditionHold);

protected:

	enum
	{
		Flag_Repeat		= 0x01,
		Flag_Waiting	= 0x02
	};

	EDeviceType	DeviceType;
	U8			Button;
	float		Time;
	float		TimeSinceDown;
	bool		Repeat;
	bool		Waiting;

public:

	CInputConditionHold(): Waiting(false) {}

	virtual bool Initialize(const Data::CParams& Desc);
	virtual void Reset() { Waiting = false; }
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
	virtual bool OnTimeElapsed(float ElapsedTime);
};

}

#endif
