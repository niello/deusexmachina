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
protected:

	EDeviceType	_DeviceType;
	U8			_Button;
	float		_Time;
	float		_TimeSinceDown;
	bool		_Repeat = false;
	bool		_Waiting = false;

public:

	CInputConditionHold(EDeviceType DeviceType, U8 Button, float Time, bool Repeat = false);

	virtual void Reset() override { _Waiting = false; }
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual bool OnTimeElapsed(float ElapsedTime) override;
};

}
