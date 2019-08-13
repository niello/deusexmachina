#pragma once
#include <Input/InputConditionState.h>

// State condition that is Off on ButtonDown and On on ButtonUp.
// Allows to check if some button is NOT pressed.

namespace Input
{

class CInputConditionReleased: public CInputConditionState
{
protected:

	EDeviceType	_DeviceType;
	U8			_Button;

public:

	CInputConditionReleased(EDeviceType DeviceType, U8 Button);

	virtual void Reset() override { On = true; }
	virtual void OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual void OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
};

}
