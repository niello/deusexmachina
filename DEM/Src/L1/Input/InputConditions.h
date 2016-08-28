#pragma once
#ifndef __DEM_L1_INPUT_CONDITIONS_H__
#define __DEM_L1_INPUT_CONDITIONS_H__

#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input event(s)

namespace Data
{
	class CParams;
}

namespace Event
{
	class AxisMove;
	class ButtonDown;
	class ButtonUp;
}

namespace Input
{
class IInputDevice;

class IInputCondition
{
public:

	virtual bool Initialize(const Data::CParams& Desc) = 0;
	virtual void Reset() {}
	virtual bool OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event, bool& Triggered) const { FAIL; }
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event, bool& Triggered) const { FAIL; }
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event, bool& Triggered) const { FAIL; }
};

class CInputConditionMove: public IInputCondition
{
protected:

	EDeviceType	DeviceType;
	U8			Axis;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual bool OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event, bool& Triggered) const;
};

class CInputConditionDown: public IInputCondition
{
protected:

	EDeviceType	DeviceType;
	U8			Button;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event, bool& Triggered) const;
};

class CInputConditionUp: public IInputCondition
{
protected:

	EDeviceType	DeviceType;
	U8			Button;

public:

	virtual bool Initialize(const Data::CParams& Desc);
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event, bool& Triggered) const;
};

}

#endif
