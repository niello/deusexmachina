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

	HANDLE			GetWin32Handle() const { return _hDevice; }
	const CString&	GetName() const { return Name; }
	virtual bool	IsOperational() const override { return Operational; }

	// For internal use only
	void			SetOperational(bool Op, HANDLE NewHandle) { Operational = Op; _hDevice = NewHandle; }
};

}
