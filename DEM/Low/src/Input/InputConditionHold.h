#pragma once
#include <Input/InputConditionEvent.h>

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

	EDeviceType	DeviceType;
	U8			Button;
	float		Time;
	float		TimeSinceDown;
	bool		Repeat = false;
	bool		Waiting = false;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual void Reset() { Waiting = false; }
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
	virtual bool OnTimeElapsed(float ElapsedTime);
};

}
