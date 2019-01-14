#if DEM_PLATFORM_WIN32
#include <System/Win32/OSFileSystemWin32.h>
#include "PlatformWin32.h"
#include <System/Win32/OSWindowWin32.h>
#include <System/Win32/MouseWin32.h>
#include <System/Win32/KeyboardWin32.h>
#include <IO/PathUtils.h>
#include <Data/StringUtils.h>
#include <shlobj.h>

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

// Window procedure for GUI windows
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLongPtr(hWnd, 0);
	LONG Result = 0;
	if (pWnd && pWnd->HandleWindowMessage(uMsg, wParam, lParam, Result)) return Result;
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

// Window procedure for a message-only window (primarily for raw input processing,
// because RIDEV_DEVNOTIFY / WM_INPUT_DEVICE_CHANGE doesn't work with NULL hWnd
LONG WINAPI MessageOnlyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CPlatformWin32* pSelf = (CPlatformWin32*)::GetWindowLongPtr(hWnd, 0);
	if (!pSelf) return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_INPUT_DEVICE_CHANGE)
	{
		if (wParam == GIDC_ARRIVAL) pSelf->OnInputDeviceArrived((HANDLE)lParam);
		else if (wParam == GIDC_REMOVAL) pSelf->OnInputDeviceRemoved((HANDLE)lParam);
		return 0;
	}
	else if (uMsg == WM_INPUT)
	{
		UINT DataSize;
		if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &DataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1 ||
			!DataSize)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		if (DataSize > pSelf->RawInputBufferSize)
		{
			pSelf->pRawInputBuffer = n_realloc(pSelf->pRawInputBuffer, DataSize);
			pSelf->RawInputBufferSize = DataSize;
		}

		if (!pSelf->pRawInputBuffer ||
			::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pSelf->pRawInputBuffer, &DataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		// We don't return after processing a WM_INPUT, instead we proceed
		// to ::DefWindowProc to perform a cleanup (see WM_INPUT docs)

		PRAWINPUT pData = (PRAWINPUT)pSelf->pRawInputBuffer;
		bool Handled = false;
		const bool IsForeground = (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT); // Can enable background input for some cases
		if (IsForeground)
		{
			for (auto& Device : pSelf->InputDevices)
			{
				if (Device->GetWin32Handle() == pData->header.hDevice)
				{
					n_assert_dbg(Device->IsOperational());
					Handled = Device->HandleRawInput(*pData);
					break;
				}
			}
		}

		// Pass to the default procedure if was not processed
		if (!Handled)
		{
			::DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));

			// Keyboard must generate legacy messages only if no one processed a raw input.
			// We don't count repeats here but it can be added if necessary. There is no much profit
			// from a correct repeat count since we generate a separate message for each repeat.
			if (pData->header.dwType == RIM_TYPEKEYBOARD)
			{
				// For now we always assume key was not down. Might be fixed if necessary.
				constexpr bool WasDown = false;

				HWND hWndFocus = ::GetFocus();
				RAWKEYBOARD& KbData = pData->data.keyboard;

				LPARAM KbLParam = (1 << 0) | (KbData.MakeCode << 16);
				if (KbData.Flags & RI_KEY_E0) KbLParam |= (1 << 24);

				switch (KbData.Message)
				{
					case WM_KEYDOWN:
					{
						if (WasDown) KbLParam |= (1 << 30);
						break;
					}
					case WM_KEYUP:
					{
						KbLParam |= ((1 << 30) | (1 << 31));
						break;
					}
					case WM_SYSKEYDOWN:
					{
						if (hWndFocus) KbLParam |= (1 << 29);
						if (WasDown) KbLParam |= (1 << 30);
						break;
					}
					case WM_SYSKEYUP:
					{
						KbLParam |= ((1 << 30) | (1 << 31));
						if (hWndFocus) KbLParam |= (1 << 29);
						break;
					}
					default:
					{
						::Sys::Error("MessageOnlyWindowProc() > unknown raw input keyboard message " + StringUtils::FromInt(KbData.Message) + '\n');
						return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
				}

				HWND hWndReceiver = hWndFocus ? hWndFocus : ::GetActiveWindow();
				if (hWndReceiver) ::PostMessage(hWndReceiver, KbData.Message, KbData.VKey, KbLParam);
			}
		}
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

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
	SAFE_FREE(pRawInputBuffer);

	UnregisterRawInput();

	if (hWndMessageOnly)
	{
		::DestroyWindow(hWndMessageOnly);
		hWndMessageOnly = 0;
	}

	if (aGUIWndClass)
	{
		if (!::UnregisterClass((const char*)aGUIWndClass, hInst))
			::Sys::Error("CPlatformWin32::~CPlatformWin32() > UnregisterClass(aGUIWndClass) failed!\n");
		aGUIWndClass = 0;
	}

	if (aMessageOnlyWndClass)
	{
		if (!::UnregisterClass((const char*)aMessageOnlyWndClass, hInst))
			::Sys::Error("CPlatformWin32::~CPlatformWin32() > UnregisterClass(aMessageOnlyWndClass) failed!\n");
		aMessageOnlyWndClass = 0;
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

bool CPlatformWin32::RegisterRawInput()
{
	if (RawInputRegistered) OK;

	if (!aMessageOnlyWndClass)
	{
		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WndClass));
		WndClass.cbSize        = sizeof(WndClass);
		WndClass.style         = 0;
		WndClass.lpfnWndProc   = MessageOnlyWindowProc;
		WndClass.cbClsExtra    = 0;
		WndClass.cbWndExtra    = sizeof(void*); // used to hold 'this' pointer
		WndClass.hInstance     = hInst;
		WndClass.hIcon         = NULL;
		WndClass.hIconSm       = NULL;
		WndClass.hCursor       = NULL;
		WndClass.hbrBackground = NULL;
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = "DeusExMachina::MessageOnlyWindow";
		aMessageOnlyWndClass = ::RegisterClassEx(&WndClass);

		if (!aMessageOnlyWndClass) FAIL;
	}

	if (!hWndMessageOnly)
	{
		hWndMessageOnly = ::CreateWindowEx(0, (const char*)aMessageOnlyWndClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);
		if (!hWndMessageOnly) FAIL;
		::SetWindowLongPtr(hWndMessageOnly, 0, (LONG_PTR)this);
	}

	RAWINPUTDEVICE RawInputDevices[3];

	RawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	RawInputDevices[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
	RawInputDevices[0].hwndTarget = hWndMessageOnly;

	// Keyboards use RIDEV_NOLEGACY, key messages are generated by engine itself when necessary
	// RIDEV_NOHOTKEYS disables Windows, Application, Ctrl+Shift+Esc
	// TODO: https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/disabling-shortcut-keys-in-games
	RawInputDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	RawInputDevices[1].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY | RIDEV_NOLEGACY | RIDEV_NOHOTKEYS;
	RawInputDevices[1].hwndTarget = hWndMessageOnly;

	RawInputDevices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
	RawInputDevices[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
	RawInputDevices[2].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
	RawInputDevices[2].hwndTarget = hWndMessageOnly;

	RawInputRegistered = (::RegisterRawInputDevices(RawInputDevices, 3, sizeof(RAWINPUTDEVICE)) != FALSE);
	return RawInputRegistered;
}
//---------------------------------------------------------------------

bool CPlatformWin32::UnregisterRawInput()
{
	if (!RawInputRegistered) OK;

	RawInputRegistered = false;

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

	return ::RegisterRawInputDevices(RawInputDevices, 3, sizeof(RAWINPUTDEVICE)) != FALSE;
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
			InputDevices.push_back(Device);
			OK;
		}
		case RIM_TYPEKEYBOARD:
		{
			Input::PKeyboardWin32 Device = n_new(Input::CKeyboardWin32);
			if (!Device->Init(hDevice, DeviceName, DeviceInfo.keyboard)) FAIL;
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
			Device->SetOperational(false, 0);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

UPTR CPlatformWin32::EnumInputDevices(CArray<Input::PInputDevice>& Out)
{
	RegisterRawInput();

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

	const UPTR PrevSize = Out.GetCount();
	for (auto& Device : InputDevices)
	{
		if (Device->IsOperational()) Out.Add(Device);
	}
	return Out.GetCount() - PrevSize;
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
		if (RawInputRegistered)
		{
			// Posted keyboard messages don't change the thread keyboard state so we must update it manually
			// for accelerators and some other windows internals (like alt codes Alt + Numpad NNN) to work.
			// "The status changes as a thread removes keyboard messages from its message queue" (c) Docs
			const bool IsKeyDown = (Msg.message == WM_KEYDOWN || Msg.message == WM_SYSKEYDOWN);
			const bool IsKeyUp = (Msg.message == WM_KEYUP || Msg.message == WM_SYSKEYUP);
			if (IsKeyDown || IsKeyUp)
			{
				if (Msg.wParam < 256)
				{
					BYTE Keys[256];
					::GetKeyboardState(Keys);

					BYTE KeyState = IsKeyDown ? 0x80 : 0x00;

					Keys[Msg.wParam] = KeyState;
					switch (Msg.wParam)
					{
						case VK_SHIFT:
						{
							// Setting VK_SHIFT bit 0x80 here disables a language switching hotkey Alt+Shift,
							// but it doesn't work with RIDEV_NOLEGACY anyway
							//Keys[VK_SHIFT] = 0;

							const UINT VKey = ::MapVirtualKey((Msg.lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
							Keys[VKey] = KeyState;
							break;
						}
						case VK_CONTROL:
						{
							const UINT VKey = (Msg.lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
							Keys[VKey] = KeyState;
							break;
						}
						case VK_MENU:
						{
							const UINT VKey = (Msg.lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
							Keys[VKey] = KeyState;
							break;
						}
					}

					::SetKeyboardState(Keys);
				}
			}
		}

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
