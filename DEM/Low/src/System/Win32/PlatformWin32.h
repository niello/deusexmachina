#pragma once
#if DEM_PLATFORM_WIN32
#include <System/Platform.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#include <vector>

// Win32 platform-dependent functionality

namespace Input
{
	typedef Ptr<class CInputDeviceWin32> PInputDeviceWin32;
}

namespace DEM { namespace Sys
{
class COSFileSystemWin32;

class CPlatformWin32 : public IPlatform
{
private:

	std::unique_ptr<COSFileSystemWin32>	FileSystemInterface;

	std::vector<Input::PInputDeviceWin32> InputDevices;

	double		PerfFreqMul;

	HINSTANCE	hInst;
	HANDLE		hRunOnceMutex = 0;

	ATOM		aGUIWndClass = 0;
	ATOM		aMessageOnlyWndClass = 0;
	HWND		hWndMessageOnly = 0;

	void*		pRawInputBuffer = nullptr;
	UPTR		RawInputBufferSize = 0;
	bool		RawInputRegistered = false;
	int			InputLocaleHotkey;

	bool RegisterRawInput();
	bool UnregisterRawInput();
	void ReadInputLocaleHotkey();
	bool OnInputDeviceArrived(HANDLE hDevice);
	bool OnInputDeviceRemoved(HANDLE hDevice);

	friend LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	friend LONG WINAPI MessageOnlyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	CPlatformWin32(HINSTANCE hInstance);
	~CPlatformWin32();

	virtual bool CheckAlreadyRunning(const char* pAppName) override;

	virtual double GetSystemTime() const override;

	virtual UPTR EnumInputDevices(CArray<Input::PInputDevice>& Out) override;

	virtual POSWindow CreateGUIWindow() override;
	//virtual POSConsoleWindow CreateConsoleWindow() override; // AllocConsole, SetConsoleTitle etc

	virtual IOSFileSystem* GetFileSystemInterface() const override;
	virtual bool GetSystemFolderPath(ESystemFolder Code, CString& OutPath) const override;

	virtual void Update() override;
};

}}

#endif
