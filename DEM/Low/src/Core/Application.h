#pragma once
//#include <Data/Singleton.h>
#include <Data/Ptr.h>
#include <Data/Params.h>
#include <Events/EventsFwd.h>
#include <memory>
#include <vector>

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

namespace Input
{
	class CInputTranslator;
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

	struct CUser
	{
		CStrID ID;
		Data::PParams Settings;
		std::unique_ptr<Input::CInputTranslator> Input;
	};

	Sys::IPlatform& Platform; //???use unique ptr and heap-allocated platform?

	std::unique_ptr<IO::CIOServer> IOServer; //???rename to IOService?

	Data::PParams GlobalSettings;
	Data::PParams OverrideSettings; // From a command line
	CStrID CurrentUserID;
	std::vector<CUser> ActiveUsers;

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

	Sys::IPlatform&	GetPlatform() const { return Platform; }

	IO::CIOServer&	IO() const;

	// enum profiles - fill array of IDs
	CStrID			CreateUserProfile(const char* pUserID);
	bool			DeleteUserProfile(const char* pUserID);
	UPTR			EnumUserProfiles(CArray<CStrID>& Out) const;
	CStrID			ActivateUser(CStrID UserID = CStrID::Empty);
	CStrID			GetCurrentUserID() const { return CurrentUserID; }

	Input::CInputTranslator* GetUserInput(CStrID UserID) const;

	void			ParseCommandLine(const char* pCmdLine);
	bool			LoadSettings(const char* pFilePath, bool Reload = false, CStrID UserID = CStrID::Empty); //???use stream?
	//CData GetSetting()
	//template<class T> T GetSetting()
	int				GetIntSetting(const char* pKey, int Default, CStrID UserID);
	float			GetFloatSetting(const char* pKey, float Default, CStrID UserID);
	const CString&	GetStringSetting(const char* pKey, const CString& Default, CStrID UserID);
	bool			SetFloatSetting(const char* pKey, float Value, CStrID UserID);

	bool			Run();
	bool			Update();
	void			Term();
	// Update, RequestState, RequestExit

	//allow multiple instances

	void			ExitOnWindowClosed(Sys::COSWindow* pWindow);

	//???store windows inside app?
	int				CreateRenderWindow(Render::CGPUDriver* pGPU, U32 Width, U32 Height);
	//POSConsoleWindow CreateConsoleWindow();
};

}
};
