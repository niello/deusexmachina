#pragma once
#ifndef __DEM_L1_INPUT_OS_WINDOW_KEYBOARD_H__
#define __DEM_L1_INPUT_OS_WINDOW_KEYBOARD_H__

#include <Input/InputDevice.h>
#include <Events/EventsFwd.h>

// Keyboard device that processes keyboard messages from an OS window.

namespace Sys
{
	class COSWindow;
}

namespace Input
{

class COSWindowKeyboard: public IInputDevice
{
private:

	Sys::COSWindow*	pWindow;

	DECLARE_EVENT_HANDLER(OSInput, OnOSWindowInput);

public:

	COSWindowKeyboard(): pWindow(NULL) {}

	void				Attach(Sys::COSWindow* pOSWindow, U16 Priority);

	virtual U8			GetAxisCount() const { return 0; }
	virtual U8			GetAxisCode(const char* pAlias) const { return InvalidCode; }
	virtual const char*	GetAxisAlias(U8 Code) const { return NULL; }
	virtual U8			GetButtonCount() const { return 255; }
	virtual U8			GetButtonCode(const char* pAlias) const;
	virtual const char*	GetButtonAlias(U8 Code) const;
};

}

#endif
