#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered by ButtonDown of the corresponding button

namespace Input
{

class CInputConditionDown: public CInputConditionEvent
{
protected:

	EDeviceType	_DeviceType;
	U8			_Button;

public:

	CInputConditionDown(EDeviceType DeviceType, U8 Button);

	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
};

}
