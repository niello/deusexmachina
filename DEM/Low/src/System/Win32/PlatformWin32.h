#pragma once
#if DEM_PLATFORM_WIN32
#include <System/Platform.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Platform-dependent functionality interface. Implemented per-platform / OS.

namespace DEM { namespace Sys
{

class CPlatformWin32 : public IPlatform
{
private:

	double		PerfFreqMul;
	HINSTANCE	hInst;

	ATOM		aGUIWndClass = 0;

public:

	CPlatformWin32(HINSTANCE hInstance);
	~CPlatformWin32();

	virtual double GetSystemTime() const override;

	virtual POSWindow CreateGUIWindow() override;
	//virtual POSConsoleWindow CreateConsoleWindow() override; // AllocConsole, SetConsoleTitle etc

	virtual void Update() override;
};

}}

#endif
