#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered by ButtonUp of the corresponding button

namespace Input
{

class CInputConditionUp: public CInputConditionEvent
{
	RTTI_CLASS_DECL(Input::CInputConditionUp, Input::CInputConditionEvent);

protected:

	EDeviceType	_DeviceType;
	U8			_Button;

public:

	CInputConditionUp(EDeviceType DeviceType, U8 Button);

	virtual UPTR OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;

	EDeviceType  GetDeviceType() const { return _DeviceType; }
	U8           GetButton() const { return _Button; }
};

}
