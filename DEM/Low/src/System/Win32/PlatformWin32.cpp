#if DEM_PLATFORM_WIN32
#include <System/Win32/OSFileSystemWin32.h>
#include "PlatformWin32.h"
#include <System/Win32/OSWindowWin32.h>
#include <System/Win32/MouseWin32.h>
#include <System/Win32/KeyboardWin32.h>
#include <IO/PathUtils.h>
#include <shlobj.h>

//!!!DBG TMP!
#include <Data/StringUtils.h>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC		((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE		((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_GAMEPAD
#define HID_USAGE_GENERIC_GAMEPAD	((USHORT) 0x05)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD	((USHORT) 0x06)
#endif

namespace DEM { namespace Sys
{

CPlatformWin32::CPlatformWin32(HINSTANCE hInstance)
	: hInst(hInstance)
{
	LONGLONG PerfFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	PerfFreqMul = 1.0 / PerfFreq;

	FileSystemInterface.reset(n_new(COSFileSystemWin32));

	//RIDEV_NOLEGACY - to prevent WM_KEYDOWN etc generation
	RAWINPUTDEVICE RawInputDevices[3];

	RawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	RawInputDevices[0].dwFlags = RIDEV_DEVNOTIFY;
	RawInputDevices[0].hwndTarget = NULL;

	RawInputDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	RawInputDevices[1].dwFlags = RIDEV_DEVNOTIFY;
	RawInputDevices[1].hwndTarget = NULL;

	RawInputDevices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
	RawInputDevices[2].dwFlags = RIDEV_DEVNOTIFY;
	RawInputDevices[2].hwndTarget = NULL;

	if (::RegisterRawInputDevices(RawInputDevices, 3, sizeof(RAWINPUTDEVICE)) == FALSE)
		::Sys::Log("CPlatformWin32::CPlatformWin32() > raw input device registration failed!\n");
}
//---------------------------------------------------------------------

CPlatformWin32::~CPlatformWin32()
{
	//RIDEV_NOLEGACY - to prevent WM_KEYDOWN etc generation
	RAWINPUTDEVICE RawInputDevices[3];

	RawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	RawInputDevices[0].dwFlags = RIDEV_REMOVE;
	RawInputDevices[0].hwndTarget = NULL;

	RawInputDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	RawInputDevices[1].dwFlags = RIDEV_REMOVE;
	RawInputDevices[1].hwndTarget = NULL;

	RawInputDevices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
	RawInputDevices[2].dwFlags = RIDEV_REMOVE;
	RawInputDevices[2].hwndTarget = NULL;

	if (::RegisterRawInputDevices(RawInputDevices, 3, sizeof(RAWINPUTDEVICE)) == FALSE)
		::Sys::Log("CPlatformWin32::~CPlatformWin32() > raw input device unregistration failed!\n");

	if (aGUIWndClass)
	{
		if (!UnregisterClass((const char*)aGUIWndClass, hInst))
			::Sys::Error("CPlatformWin32::~CPlatformWin32() > UnregisterClass() failed!\n");
		aGUIWndClass = 0;
	}
}
//---------------------------------------------------------------------

double CPlatformWin32::GetSystemTime() const
{
	LONGLONG PerfTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	return PerfTime * PerfFreqMul;
}
//---------------------------------------------------------------------

bool CPlatformWin32::OnInputDeviceArrived(HANDLE hDevice)
{
	// Try to find operational device by hDevice. If found, don't create a new device.

	for (auto& Device : InputDevices)
	{
		if (Device->IsOperational() && Device->GetWin32Handle() == hDevice) OK;
	}

	// Try to find non-operational device by DeviceName. If found, return it to operational state and update handle.

	char NameBuf[512];
	UINT NameBufSize = 512;
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, NameBuf, &NameBufSize) <= 0) FAIL;
	const CString DeviceName(NameBuf);

	for (auto& Device : InputDevices)
	{
		if (!Device->IsOperational() && Device->GetName() == DeviceName)
		{
			Device->SetOperational(true, hDevice);
			OK;
		}
	}

	// Device not found and must be created

	RID_DEVICE_INFO DeviceInfo;
	DeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
	UINT DevInfoSize = sizeof(RID_DEVICE_INFO);
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &DeviceInfo, &DevInfoSize) <= 0) FAIL;

	switch (DeviceInfo.dwType)
	{
		case RIM_TYPEMOUSE:
		{
			Input::PMouseWin32 Device = n_new(Input::CMouseWin32);
			if (!Device->Init(hDevice, DeviceName, DeviceInfo.mouse)) FAIL;
			::Sys::DbgOut("CPlatformWin32::OnInputDeviceArrived() > mouse added: " + DeviceName + '\n');
			InputDevices.push_back(Device);
			OK;
		}
		case RIM_TYPEKEYBOARD:
		{
			Input::PKeyboardWin32 Device = n_new(Input::CKeyboardWin32);
			if (!Device->Init(hDevice, DeviceName, DeviceInfo.keyboard)) FAIL;
			::Sys::DbgOut("CPlatformWin32::OnInputDeviceArrived() > keyboard added: " + DeviceName + '\n');
			InputDevices.push_back(Device);
			OK;
		}
		//case RIM_TYPEHID:
		//{
		//	// detect & register gamepads, skip other devices
		//	break;
		//}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPlatformWin32::OnInputDeviceRemoved(HANDLE hDevice)
{
	for (auto& Device : InputDevices)
	{
		if (Device->IsOperational() && Device->GetWin32Handle() == hDevice)
		{
			::Sys::DbgOut("CPlatformWin32::OnInputDeviceRemoved() > keyboard added: " + Device->GetName() + '\n');
			Device->SetOperational(false, 0);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

UPTR CPlatformWin32::EnumInputDevices(CArray<Input::PInputDevice>& Out)
{
	UINT Count;
	if (::GetRawInputDeviceList(NULL, &Count, sizeof(RAWINPUTDEVICELIST)) != 0 || !Count)
	{
		for (auto& Device : InputDevices)
			Device->SetOperational(false, 0);
		return 0;
	}

	PRAWINPUTDEVICELIST pList = n_new_array(RAWINPUTDEVICELIST, Count);
	if (::GetRawInputDeviceList(pList, &Count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
	{
		n_delete_array(pList);
		for (auto& Device : InputDevices)
			Device->SetOperational(false, 0);
		return 0;
	}

	for (UINT i = 0; i < Count; ++i)
	{
		OnInputDeviceArrived(pList[i].hDevice);
	}

	// Disable all operational devices that aren't present in a new list
	for (auto& Device : InputDevices)
	{
		if (!Device->IsOperational()) continue;

		bool Found = false;
		for (UINT i = 0; i < Count; ++i)
		{
			if (Device->GetWin32Handle() == pList[i].hDevice)
			{
				Found = true;
				break;
			}
		}

		if (!Found) Device->SetOperational(false, 0);
	}

	n_delete_array(pList);
	return Count;
}
//---------------------------------------------------------------------

LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLongPtr(hWnd, 0);
	LONG Result = 0;
	if (pWnd && pWnd->HandleWindowMessage(uMsg, wParam, lParam, Result)) return Result;
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

POSWindow CPlatformWin32::CreateGUIWindow()
{
	if (!aGUIWndClass)
	{
		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WndClass));
		WndClass.cbSize        = sizeof(WndClass);
		WndClass.style         = CS_DBLCLKS; // | CS_HREDRAW | CS_VREDRAW;
		WndClass.lpfnWndProc   = WindowProc;
		WndClass.cbClsExtra    = 0;
		WndClass.cbWndExtra    = sizeof(void*); // used to hold 'this' pointer
		WndClass.hInstance     = hInst;
		WndClass.hIcon         = ::LoadIcon(NULL, IDI_APPLICATION); // TODO: default DEM icon
		WndClass.hIconSm       = NULL; // set it too?
		WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = "DeusExMachina::MainWindow";
		aGUIWndClass = ::RegisterClassEx(&WndClass);
		
		if (!aGUIWndClass) return nullptr;
	}

	POSWindowWin32 Wnd = n_new(COSWindowWin32(hInst, aGUIWndClass));
	return Wnd->GetHWND() ? Wnd.Get() : nullptr;
}
//---------------------------------------------------------------------

void CPlatformWin32::Update()
{
	MSG Msg;
	while (::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (Msg.message == WM_INPUT_DEVICE_CHANGE)
		{
			if (Msg.wParam == GIDC_ARRIVAL) OnInputDeviceArrived((HANDLE)Msg.lParam);
			else if (Msg.wParam == GIDC_REMOVAL) OnInputDeviceRemoved((HANDLE)Msg.lParam);
		}
		else if (Msg.message == WM_INPUT)
		{
			// IsForeground: GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT
			RAWINPUT Data;
			UINT DataSize = sizeof(Data);
			if (::GetRawInputData((HRAWINPUT)Msg.lParam, RID_INPUT, &Data, &DataSize, sizeof(RAWINPUTHEADER)) != (UINT)-1)
			{
				for (auto& Device : InputDevices)
				{
					if (Device->IsOperational() && Device->GetWin32Handle() == Data.header.hDevice)
					{
						// pass to device along with a target window
						// if device has no clients it will skip event and generate no output
						break;
					}
				}
			}
		}
		else
		{
			// Process accelerators of our own windows
			if (aGUIWndClass && Msg.hwnd && ::GetClassWord(Msg.hwnd, GCW_ATOM) == aGUIWndClass)
			{
				// Only our windows store engine window pointer at 0
				const COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLongPtr(Msg.hwnd, 0);
				HACCEL hAccel = pWnd ? pWnd->GetWin32AcceleratorTable() : 0;
				if (hAccel && ::TranslateAccelerator(Msg.hwnd, hAccel, &Msg) != FALSE) continue;
			}

			::TranslateMessage(&Msg);
			::DispatchMessage(&Msg);
		}
	}
}
//---------------------------------------------------------------------

IOSFileSystem* CPlatformWin32::GetFileSystemInterface() const
{
	return FileSystemInterface.get();
}
//---------------------------------------------------------------------

bool CPlatformWin32::GetSystemFolderPath(ESystemFolder Code, CString& OutPath) const
{
	char pRawPath[DEM_MAX_PATH];
	if (Code == SysFolder_Temp)
	{
		if (!::GetTempPath(sizeof(pRawPath), pRawPath)) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}
	else if (Code == SysFolder_Bin || Code == SysFolder_Home)
	{
		if (!::GetModuleFileName(NULL, pRawPath, sizeof(pRawPath))) FAIL;
		CString PathToExe(pRawPath);
		PathToExe.Replace('\\', '/');
		OutPath = PathUtils::CollapseDots(PathUtils::ExtractDirName(PathToExe));
	}
	else if (Code == SysFolder_WorkingDir)
	{
		if (!::GetCurrentDirectory(sizeof(pRawPath), pRawPath)) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}
	else
	{
		int CSIDL;
		switch (Code)
		{
			case SysFolder_User:		CSIDL = CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE; break;
			case SysFolder_AppData:		CSIDL = CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE; break;
			case SysFolder_Programs:	CSIDL = CSIDL_PROGRAM_FILES; break;
			default:					FAIL;
		}

		if (FAILED(::SHGetFolderPath(0, CSIDL, NULL, 0, pRawPath))) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}

	PathUtils::EnsurePathHasEndingDirSeparator(OutPath);

	OK;
}
//---------------------------------------------------------------------

}};

#endif
