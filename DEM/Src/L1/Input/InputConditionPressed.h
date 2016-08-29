#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_PRESSED_H__
#define __DEM_L1_INPUT_CONDITION_PRESSED_H__

#include <Input/InputCondition.h>

// State condition that is On on ButtonDown and Off on ButtonUp

namespace Input
{

class CInputConditionPressed: public CInputConditionState
{
	__DeclareClass(CInputConditionPressed);

protected:

	EDeviceType	DeviceType;
	U8			Button;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual void Reset() { On = false; }
	virtual void OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual void OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
};

}

#endif
