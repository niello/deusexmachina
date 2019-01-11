#pragma once
//#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Events/EventsFwd.h>
#include <memory>

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

namespace Render
{
	class CGPUDriver;
}

namespace IO
{
	class CIOServer;
}

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

	std::unique_ptr<IO::CIOServer> IOServer; //???rename to IOService?

	double BaseTime = 0.0;
	double PrevTime = 0.0;
	double FrameTime = 0.0;
	float TimeScale = 1.f;

	//!!!DBG TMP! Will be state!
	bool Exiting = false;

	DECLARE_EVENT_HANDLER(OnClosing, OnMainWindowClosing);

public:

	CApplication(Sys::IPlatform& _Platform);
	virtual ~CApplication();

	Sys::IPlatform& GetPlatform() const { return Platform; }

	IO::CIOServer& IO() const;

	bool Run();
	bool Update();
	void Term();
	// Update, RequestState, RequestExit

	//allow multiple instances
	//exit when last window closed

	void ExitOnWindowClosed(Sys::COSWindow* pWindow);

	//???store windows inside app?
	int CreateRenderWindow(Render::CGPUDriver* pGPU, U32 Width, U32 Height);
	//POSConsoleWindow CreateConsoleWindow();
};

}
};
