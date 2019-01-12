#pragma once
#if DEM_PLATFORM_WIN32
#include <System/Platform.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>

// Win32 platform-dependent functionality

namespace DEM { namespace Sys
{
class COSFileSystemWin32;

class CPlatformWin32 : public IPlatform
{
private:

	std::unique_ptr<COSFileSystemWin32>	FileSystemInterface;

	double		PerfFreqMul;
	HINSTANCE	hInst;

	ATOM		aGUIWndClass = 0;

public:

	CPlatformWin32(HINSTANCE hInstance);
	~CPlatformWin32();

	virtual double GetSystemTime() const override;

	virtual UPTR EnumInputDevices(CArray<Input::IInputDevice*>& Out) override;

	virtual POSWindow CreateGUIWindow() override;
	//virtual POSConsoleWindow CreateConsoleWindow() override; // AllocConsole, SetConsoleTitle etc

	virtual IOSFileSystem* GetFileSystemInterface() const override;
	virtual bool GetSystemFolderPath(ESystemFolder Code, CString& OutPath) const override;

	virtual void Update() override;
};

}}

#endif
