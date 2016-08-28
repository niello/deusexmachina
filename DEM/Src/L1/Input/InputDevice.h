#pragma once
#ifndef __DEM_L1_INPUT_DEVICE_H__
#define __DEM_L1_INPUT_DEVICE_H__

#include <Input/InputFwd.h>
#include <Events/EventDispatcher.h>

// An interface for various input devices such as keyboards, mouses and gamepads.
// Converts specific input to AxisMove, ButtonDown and ButtonUp native events.

namespace Input
{

class IInputDevice: public Events::CEventDispatcher
{
private:

public:

	static const U8 InvalidCode = 0xff;

	virtual EDeviceType	GetType() const = 0;
	virtual U8			GetAxisCount() const = 0;
	virtual U8			GetAxisCode(const char* pAlias) const = 0;
	virtual const char*	GetAxisAlias(U8 Code) const = 0;
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity) = 0;
	virtual float		GetAxisSensitivity(U8 Code) const = 0;
	virtual U8			GetButtonCount() const = 0;
	virtual U8			GetButtonCode(const char* pAlias) const = 0;
	virtual const char*	GetButtonAlias(U8 Code) const = 0;
};

}

#endif
