#pragma once
#ifndef __DEM_L1_INPUT_OS_WINDOW_KEYBOARD_H__
#define __DEM_L1_INPUT_OS_WINDOW_KEYBOARD_H__

#include <Input/InputDevice.h>
#include <Events/EventsFwd.h>

// Keyboard device that processes keyboard messages from an OS window.

namespace DEM { namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}}

namespace Input
{

class COSWindowKeyboard: public IInputDevice
{
private:

	DEM::Sys::COSWindow*	pWindow = nullptr;

	DECLARE_EVENT_HANDLER(OSInput, OnOSWindowInput);

public:

	void				Attach(DEM::Sys::COSWindow* pOSWindow, U16 Priority);

	virtual EDeviceType	GetType() const { return Dev_Keyboard; }
	virtual U8			GetAxisCount() const { return 0; }
	virtual U8			GetAxisCode(const char* pAlias) const { return InvalidCode; }
	virtual const char*	GetAxisAlias(U8 Code) const { return NULL; }
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity) {}
	virtual float		GetAxisSensitivity(U8 Code) const { return 1.f; }
	virtual U8			GetButtonCount() const { return 255; }
	virtual U8			GetButtonCode(const char* pAlias) const;
	virtual const char*	GetButtonAlias(U8 Code) const;
};

}

#endif
