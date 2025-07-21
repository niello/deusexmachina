#pragma once
#include <System/Win32/InputDeviceWin32.h>
#include <memory>

// Win32 mouse device implementation

namespace Input
{
typedef Ptr<class CMouseWin32> PMouseWin32;

class CMouseWin32: public CInputDeviceWin32
{
protected:

	DWORD						ID = 0;
	std::unique_ptr<float[]>	AxisSensitivity;
	U32							AxisCount = 0;
	U32							ButtonCount = 0;

public:

	CMouseWin32();
	virtual ~CMouseWin32();

	bool				Init(HANDLE hDevice, const std::string& DeviceName, const RID_DEVICE_INFO_MOUSE& DeviceInfo);

	virtual bool		CanHandleRawInput(const RAWINPUT& Data) const override;
	virtual bool		HandleRawInput(const RAWINPUT& Data) override;

	virtual EDeviceType	GetType() const override { return Device_Mouse; }
	virtual U8			GetAxisCount() const override { return AxisCount; }
	virtual U8			GetAxisCode(const char* pAlias) const override;
	virtual const char*	GetAxisAlias(U8 Code) const override;
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity) override;
	virtual float		GetAxisSensitivity(U8 Code) const override;
	virtual U8			GetButtonCount() const override { return ButtonCount; }
	virtual U8			GetButtonCode(const char* pAlias) const override;
	virtual const char*	GetButtonAlias(U8 Code) const override;
	virtual bool		CanInputText() const override { FAIL; }
};

}
