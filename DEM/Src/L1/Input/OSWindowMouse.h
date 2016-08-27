#pragma once
#ifndef __DEM_L1_INPUT_OS_WINDOW_MOUSE_H__
#define __DEM_L1_INPUT_OS_WINDOW_MOUSE_H__

#include <Input/InputDevice.h>
#include <Events/EventsFwd.h>

// Mouse device that processes mouse messages from an OS window.
// Supports up to 4 axes (x, y and 2 wheels) and up to 6 buttons (L, R, M, X1, X2, any other).

namespace Sys
{
	class COSWindow;
}

namespace Input
{

class COSWindowMouse: public IInputDevice
{
private:

	static const U8 AxisCount = 4;

	Sys::COSWindow*	pWindow;
	float			AxisSensitivity[AxisCount];

	DECLARE_EVENT_HANDLER(OSInput, OnOSWindowInput);

public:

	COSWindowMouse();

	void				Attach(Sys::COSWindow* pOSWindow, U16 Priority);

	virtual EDeviceType	GetType() const { return Dev_Mouse; }
	virtual U8			GetAxisCount() const { return AxisCount; }
	virtual U8			GetAxisCode(const char* pAlias) const;
	virtual const char*	GetAxisAlias(U8 Code) const;
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity);
	virtual float		GetAxisSensitivity(U8 Code) const;
	virtual U8			GetButtonCount() const { return 6; }
	virtual U8			GetButtonCode(const char* pAlias) const;
	virtual const char*	GetButtonAlias(U8 Code) const;
};

}

#endif
