#pragma once
#include <System/Win32/InputDeviceWin32.h>

// Win32 keyboard device implementation

namespace Input
{
typedef Ptr<class CKeyboardWin32> PKeyboardWin32;

class CKeyboardWin32: public CInputDeviceWin32
{
protected:

	DWORD	Type = 0;
	DWORD	Subtype = 0;
	U32		ButtonCount = 0;

public:

	CKeyboardWin32();
	virtual ~CKeyboardWin32();

	bool				Init(HANDLE hDevice, const std::string& DeviceName, const RID_DEVICE_INFO_KEYBOARD& DeviceInfo);

	virtual bool		CanHandleRawInput(const RAWINPUT& Data) const override;
	virtual bool		HandleRawInput(const RAWINPUT& Data) override;
	virtual bool		HandleCharMessage(WPARAM Char) override;

	virtual EDeviceType	GetType() const override { return Device_Keyboard; }
	virtual U8			GetAxisCount() const override { return 0; }
	virtual U8			GetAxisCode(const char* pAlias) const override { return InvalidCode; }
	virtual const char*	GetAxisAlias(U8 Code) const override { return nullptr; }
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity) override {}
	virtual float		GetAxisSensitivity(U8 Code) const override { return 0.f; }
	virtual U8			GetButtonCount() const override { return ButtonCount; }
	virtual U8			GetButtonCode(const char* pAlias) const override;
	virtual const char*	GetButtonAlias(U8 Code) const override;
	virtual bool		CanInputText() const override { OK; }
};

}
