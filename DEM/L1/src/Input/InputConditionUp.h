#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_UP_H__
#define __DEM_L1_INPUT_CONDITION_UP_H__

#include <Input/InputCondition.h>

// Event condition that is triggered by ButtonUp of the corresponding button

namespace Input
{

class CInputConditionUp: public CInputConditionEvent
{
	__DeclareClass(CInputConditionUp);

protected:

	EDeviceType	DeviceType;
	U8			Button;

public:

	virtual bool	Initialize(const Data::CParams& Desc);
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);

	EDeviceType		GetDeviceType() const { return DeviceType; }
	U8				GetButton() const { return Button; }
};

}

#endif
