#pragma once
#ifndef __DEM_L1_INPUT_FWD_H__
#define __DEM_L1_INPUT_FWD_H__

#include <StdDEM.h>

// Input system forward declarations

//???rename to keys/keycodes?

namespace Input
{

enum EDeviceType
{
	Dev_Keyboard,
	Dev_Mouse,
	Dev_Gamepad
};

enum EMouseButton
{
	MB_Left		= 0,
	MB_Right	= 1,
	MB_Middle	= 2,
	MB_X1		= 3,
	MB_X2		= 4,
	MB_Other	= 5	// User values (MB_Other + N), N = 0 .. X
};

const char*		MouseButtonToString(EMouseButton Button);
EMouseButton	StringToMouseButton(const char* pName);

}

#endif
