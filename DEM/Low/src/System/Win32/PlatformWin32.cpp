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

namespace DEM { namespace Sys
{

CPlatformWin32::CPlatformWin32(HINSTANCE hInstance)
	: hInst(hInstance)
{
	LONGLONG PerfFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	PerfFreqMul = 1.0 / PerfFreq;

	FileSystemInterface.reset(n_new(COSFileSystemWin32));
}
//---------------------------------------------------------------------

CPlatformWin32::~CPlatformWin32()
{
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
			if (!Device->Init(hDevice)) FAIL;
			InputDevices.push_back(Device);
			OK;
		}
		case RIM_TYPEKEYBOARD:
		{
			Input::PKeyboardWin32 Device = n_new(Input::CKeyboardWin32);
			if (!Device->Init(hDevice)) FAIL;
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

	//???remove all non-operational devices without users?
	//!!!check operational devices with users & register / unregister raw input appropriately!

	n_delete_array(pList);
	return Count;
}
//---------------------------------------------------------------------

LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(hWnd, 0);
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
			HANDLE hDevice = (HANDLE)Msg.lParam;
			//if (Msg.wParam == GIDC_ARRIVAL) OnInputDeviceArrived(hDevice);
			//else if (Msg.wParam == GIDC_REMOVAL) OnInputDeviceRemoved(hDevice);

			//!!!DBG TMP!
			::Sys::DbgOut(CString("***DBG CPlatformWin32::Update() > WM_INPUT_DEVICE_CHANGE ") + StringUtils::FromInt((int)Msg.hwnd) + "\n");

			continue;
		}

		// Process accelerators of our own windows
		if (aGUIWndClass && Msg.hwnd)
		{
			ATOM aClass = ::GetClassWord(Msg.hwnd, GCW_ATOM);
			if (aClass == aGUIWndClass)
			{
				// Only our windows store engine window pointer at 0
				const COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(Msg.hwnd, 0);
				HACCEL hAccel = pWnd ? pWnd->GetWin32AcceleratorTable() : 0;
				if (hAccel && ::TranslateAccelerator(Msg.hwnd, hAccel, &Msg) != FALSE) continue;
			}
		}

		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
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
