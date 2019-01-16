#if DEM_PLATFORM_WIN32
#include <System/Win32/OSFileSystemWin32.h>
#include "PlatformWin32.h"
#include <System/Win32/OSWindowWin32.h>
#include <System/Win32/MouseWin32.h>
#include <System/Win32/KeyboardWin32.h>
#include <IO/PathUtils.h>
#include <Data/StringUtils.h>

#include <shlobj.h>
#define SECURITY_WIN32
#include <Security.h>

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

// Keyboard raw input with RIDEV_NOLEGACY kills language switching hotkey, so we reimplement it by hand
constexpr int INPUT_LOCALE_HOTKEY_ALT_SHIFT = 1;
constexpr int INPUT_LOCALE_HOTKEY_CTRL_SHIFT = 2;
constexpr int INPUT_LOCALE_HOTKEY_NOT_SET = 3;
constexpr int INPUT_LOCALE_HOTKEY_GRAVE = 4;

namespace DEM { namespace Sys
{

static void SetKeyState(BYTE& KeyState, bool IsDown)
{
	// Set or clear 'down' bit 0x80, on down toggle 'toggle' bit 0x01
	if (IsDown) KeyState = 0x80 | (0x01 ^ (KeyState & 0x01));
	else KeyState &= ~0x80;
}
//---------------------------------------------------------------------

// Window procedure for GUI windows
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Some app-wide messages are sent to window directly, so we intercept them here
	switch (uMsg)
	{
		case WM_ACTIVATEAPP:
		{
			CPlatformWin32* pPlatform = (CPlatformWin32*)::GetWindowLongPtr(hWnd, sizeof(void*));
			if (pPlatform && pPlatform->RawInputRegistered) pPlatform->ReadInputLocaleHotkey();
			break;
		}
		case WM_SETTINGCHANGE:
		{
			if (wParam == SPI_SETLANGTOGGLE)
			{
				CPlatformWin32* pPlatform = (CPlatformWin32*)::GetWindowLongPtr(hWnd, sizeof(void*));
				if (pPlatform && pPlatform->RawInputRegistered) pPlatform->ReadInputLocaleHotkey();
			}
			break;
		}
		case WM_INPUTLANGCHANGE:
		{
			//::Sys::DbgOut("Language changed\n");
			break;
		}
	}

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
			LRESULT lr = ::DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			if (lr != S_OK)
			{
				n_assert2_dbg(false, "MessageOnlyWindowProc() > raw input default processing error");
				return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
			}

			// Keyboard must generate legacy messages only if no one processed a raw input.
			// We don't count repeats here but it can be added if necessary. There is no much profit
			// from a correct repeat count since we generate a separate message for each repeat.
			if (pData->header.dwType == RIM_TYPEKEYBOARD)
			{
				// For now we always assume key was not down. Might be fixed if necessary.
				constexpr bool WasDown = false;

				HWND hWndFocus = ::GetFocus();
				RAWKEYBOARD& KbData = pData->data.keyboard;
				const SHORT VKey = KbData.VKey;
				const BYTE ScanCode = static_cast<BYTE>(KbData.MakeCode);
				const bool IsExt = (KbData.Flags & RI_KEY_E0);

				LPARAM KbLParam = (1 << 0) | (ScanCode << 16);
				if (IsExt) KbLParam |= (1 << 24);

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
						return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
				}

				HWND hWndReceiver = hWndFocus ? hWndFocus : ::GetActiveWindow();
				if (hWndReceiver) ::PostMessage(hWndReceiver, KbData.Message, VKey, KbLParam);

				// Posted keyboard messages don't change the thread keyboard state so we must update it manually
				// for accelerators and some other windows internals (like alt codes Alt + Numpad NNN) to work.
				if (VKey < 256)
				{
					BYTE Keys[256];
					::GetKeyboardState(Keys);

					const bool IsKeyDown = (KbData.Message == WM_KEYDOWN || KbData.Message == WM_SYSKEYDOWN);
					SetKeyState(Keys[VKey], IsKeyDown);

					switch (VKey)
					{
						case VK_SHIFT:
						{
							const UINT VKeyH = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
							SetKeyState(Keys[VKeyH == VK_RSHIFT ? VK_RSHIFT : VK_LSHIFT], IsKeyDown);
							break;
						}
						case VK_CONTROL:
						{
							const SHORT VKeyH = IsExt ? VK_RCONTROL : VK_LCONTROL;
							SetKeyState(Keys[VKeyH], IsKeyDown);
							break;
						}
						case VK_MENU:
						{
							const SHORT VKeyH = IsExt ? VK_RMENU : VK_LMENU;
							SetKeyState(Keys[VKeyH], IsKeyDown);
							break;
						}
					}

					::SetKeyboardState(Keys);
				}
			}
		}
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

CPlatformWin32::CPlatformWin32(HINSTANCE hInstance)
	: hInst(hInstance)
	, InputLocaleHotkey(INPUT_LOCALE_HOTKEY_NOT_SET)
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

	if (hRunOnceMutex)
	{
		::CloseHandle(hRunOnceMutex);
		hRunOnceMutex = 0;
	}
}
//---------------------------------------------------------------------

CString CPlatformWin32::GetOSUserName() const
{
	char Buf[256];
	DWORD BufSize = sizeof(Buf);
	if (!::GetUserNameEx(NameDisplay, Buf, &BufSize))
	{
		if (::GetLastError() != 1332) return CString();
		if (!::GetUserName(Buf, &BufSize)) return CString();
	}
	return CString(Buf);
}
//---------------------------------------------------------------------

bool CPlatformWin32::CheckAlreadyRunning(const char* pAppName)
{
	// Cant check an app without name
	if (!pAppName || !*pAppName) FAIL;

	// We are the first instance, and we do't want to detect ourselves as a conflicting process
	if (hRunOnceMutex) FAIL;

	CString Prefix("DEM::CPlatformWin32::CheckAlreadyRunning::");
	hRunOnceMutex = ::CreateMutex(NULL, TRUE, Prefix + pAppName);
	if (hRunOnceMutex && ::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// The same app is already running
		::CloseHandle(hRunOnceMutex);
		hRunOnceMutex = 0;
		OK;
	}

	// We are the first instance, so keep mutex handle
	FAIL;
}
//---------------------------------------------------------------------

double CPlatformWin32::GetSystemTime() const
{
	LONGLONG PerfTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	return PerfTime * PerfFreqMul;
}
//---------------------------------------------------------------------

void CPlatformWin32::ReadInputLocaleHotkey()
{
	InputLocaleHotkey = INPUT_LOCALE_HOTKEY_ALT_SHIFT; // Windows default setting

	char RegData[4];
	DWORD RegDataSize = sizeof(RegData);
	LSTATUS ls = ::RegGetValue(HKEY_CURRENT_USER, "Keyboard Layout\\Toggle", "Hotkey", RRF_RT_ANY, NULL, &RegData, &RegDataSize);
	if (ls == ERROR_SUCCESS && RegDataSize > 0)
	{
		switch (RegData[0])
		{
			case '1': InputLocaleHotkey = INPUT_LOCALE_HOTKEY_ALT_SHIFT; break;
			case '2': InputLocaleHotkey = INPUT_LOCALE_HOTKEY_CTRL_SHIFT; break;
			case '3': InputLocaleHotkey = INPUT_LOCALE_HOTKEY_NOT_SET; break;
			case '4': InputLocaleHotkey = INPUT_LOCALE_HOTKEY_GRAVE; break;
		};
	}
}
//---------------------------------------------------------------------

bool CPlatformWin32::RegisterRawInput()
{
	if (RawInputRegistered) OK;

	ReadInputLocaleHotkey();

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
		WndClass.cbWndExtra    = sizeof(void*) + sizeof(void*); // used to hold COSWindowWin32 and CPlatformWin32 'this' pointers
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
	if (!Wnd->GetHWND()) return nullptr;

	::SetWindowLongPtr(Wnd->GetHWND(), sizeof(void*), (LONG_PTR)this);

	return Wnd.Get();
}
//---------------------------------------------------------------------

void CPlatformWin32::Update()
{
	MSG Msg;
	while (::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		// Restore input locale switching hotkey logic killed by raw input.
		// Accelerators can't process Alt+Shift or Ctrl+Shift, also accelerators are
		// triggered by keydown. So we implement a correct behaviour manually.
		if (RawInputRegistered && InputLocaleHotkey != INPUT_LOCALE_HOTKEY_NOT_SET)
		{
			if (Msg.message == WM_KEYDOWN || Msg.message == WM_SYSKEYDOWN)
			{
				// Suppress character generation
				if (InputLocaleHotkey == INPUT_LOCALE_HOTKEY_GRAVE && Msg.wParam == VK_OEM_3) continue;
			}
			else if (Msg.message == WM_KEYUP || Msg.message == WM_SYSKEYUP)
			{
				bool ToggleInputLocale = false;
				switch (InputLocaleHotkey)
				{
					case INPUT_LOCALE_HOTKEY_ALT_SHIFT:
						ToggleInputLocale = (Msg.wParam == VK_SHIFT && (::GetKeyState(VK_MENU) & 0x80));
						break;
					case INPUT_LOCALE_HOTKEY_CTRL_SHIFT:
						ToggleInputLocale = (Msg.wParam == VK_SHIFT && (::GetKeyState(VK_CONTROL) & 0x80));
						break;
					case INPUT_LOCALE_HOTKEY_GRAVE:
						ToggleInputLocale = (Msg.wParam == VK_OEM_3); // Grave ('`', typically the same key as '~')
						break;
				}

				if (ToggleInputLocale)
				{
					::ActivateKeyboardLayout((HKL)HKL_NEXT, KLF_SETFORPROCESS);
					continue;
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
