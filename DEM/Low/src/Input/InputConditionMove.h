#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered by AxisMove of the corresponding axis.
// If threshold is set, event is fired only when amount of movement exceeds it.
// Event is fired repeatedly.

namespace Input
{

class CInputConditionMove: public CInputConditionEvent
{
protected:

	EDeviceType	_DeviceType;
	U8			_Axis;
	float		_Threshold = 0.f;
	float		_Accumulated = 0.f;

public:

	CInputConditionMove(EDeviceType DeviceType, U8 Axis, float Threshold = 0.f);

	virtual bool OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
};

}
