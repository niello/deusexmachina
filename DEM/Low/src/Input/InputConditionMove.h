#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_MOVE_H__
#define __DEM_L1_INPUT_CONDITION_MOVE_H__

#include <Input/InputCondition.h>

// Event condition that is triggered by AxisMove of the corresponding axis

namespace Input
{

class CInputConditionMove: public CInputConditionEvent
{
	__DeclareClass(CInputConditionMove);

protected:

	EDeviceType	DeviceType;
	U8			Axis;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual bool OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event);
};

}

#endif
