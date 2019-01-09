#pragma once
//#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Data/Array.h>
#include <Events/EventsFwd.h>
//#include <vector>

// DEM application base class. Application serves as a state machine,
// OS interface and a global service container.

// OS-specific:
// - file IO
// - time
// - memory
// - window (system GUI & input)
// - threads

// DEM:
// - app settings
// - global variables
// - events
// - user profiles (settings & saves)
// - time sources (need named timers? use delayed events with handles?)
// - application states (FSM)
// - callbacks / virtual methods for application lifecycle control in derived applications
// - factory

namespace DEM
{
namespace Sys
{
	class IPlatform;
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Core
{

class CApplication
{
protected:

	Sys::IPlatform& Platform; //???use unique ptr and heap-allocated platform?

	//CArray<Sys::POSWindow> Windows;

	double BaseTime = 0.0;
	double PrevTime = 0.0;
	double FrameTime = 0.0;
	float TimeScale = 1.f;

	//!!!DBG TMP! Will be state!
	bool Exiting = false;

	DECLARE_EVENT_HANDLER(OnClosing, OnMainWindowClosing);

public:

	CApplication(Sys::IPlatform& _Platform);

	Sys::IPlatform& GetPlatform() const { return Platform; }

	bool Run();
	bool Update();
	void Close();
	// Update, RequestState, RequestExit

	//allow multiple instances
	//exit when last window closed

	void ExitOnWindowClosed(Sys::COSWindow* pWindow);

	//???store windows inside app?
	Sys::POSWindow CreateRenderWindow(); // bool close app on close this window?
	//POSConsoleWindow CreateConsoleWindow(); // bool close app on close this window?
};

}
};
