#pragma once
#include <Input/InputConditionState.h>

// State condition that is On on ButtonDown and Off on ButtonUp

namespace Input
{

class CInputConditionPressed: public CInputConditionState
{
protected:

	EDeviceType	_DeviceType;
	U8			_Button;

public:

	CInputConditionPressed(EDeviceType DeviceType, U8 Button);

	virtual void Reset() override { On = false; }
	virtual void OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual void OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
};

}
