#pragma once
#ifndef __DEM_L1_INPUT_FWD_H__
#define __DEM_L1_INPUT_FWD_H__

#include <StdDEM.h>

// Input system forward declarations

namespace Input
{
class IInputDevice;

enum EDeviceType
{
	Dev_Keyboard,
	Dev_Mouse,
	Dev_Gamepad,

	Dev_Count,
	Dev_Invalid
};

const char*	DeviceTypeToString(EDeviceType Type);
EDeviceType	StringToDeviceType(const char* pName);

enum EMouseAxis
{
	MA_X		= 0,
	MA_Y		= 1,
	MA_Wheel1	= 2,
	MA_Wheel2	= 3,

	MA_Count,
	MA_Invalid
};

const char*	MouseAxisToString(EMouseAxis Axis);
EMouseAxis	StringToMouseAxis(const char* pName);

enum EMouseButton
{
	MB_Left		= 0,
	MB_Right	= 1,
	MB_Middle	= 2,
	MB_X1		= 3,
	MB_X2		= 4,
	MB_User		= 5,	// User values (MB_User + N), N = 0 .. X

	MB_Invalid	= 255
};

const char*		MouseButtonToString(EMouseButton Button);
EMouseButton	StringToMouseButton(const char* pName);

}

namespace Event
{
	class AxisMove;
	class ButtonDown;
	class ButtonUp;
}

#endif
