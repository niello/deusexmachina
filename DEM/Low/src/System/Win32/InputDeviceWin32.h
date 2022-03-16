#pragma once
#include <Input/InputDevice.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 input device base class

namespace Input
{
typedef Ptr<class CInputDeviceWin32> PInputDeviceWin32;

class CInputDeviceWin32: public IInputDevice
{
protected:

	CString	Name;
	HANDLE	_hDevice = 0;
	bool	Operational = false;

public:

	HANDLE				GetWin32Handle() const { return _hDevice; }
	virtual const char*	GetName() const override { return Name.CStr(); }
	virtual bool		IsOperational() const override { return Operational; }

	virtual bool		CanHandleRawInput(const RAWINPUT& Data) const = 0;
	virtual bool		HandleRawInput(const RAWINPUT& Data) = 0;
	virtual bool		HandleCharMessage(WPARAM Char) { FAIL; }

	// For internal use only
	void				SetOperational(bool Op, HANDLE NewHandle) { Operational = Op; _hDevice = NewHandle; }
};

}
