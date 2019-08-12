#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered by ButtonDown of the corresponding button

namespace Input
{

class CInputConditionDown: public CInputConditionEvent
{
	__DeclareClass(CInputConditionDown);

protected:

	EDeviceType	DeviceType;
	U8			Button;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
};

}
