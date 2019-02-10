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

namespace Resources
{
	class CResourceManager;
}

namespace Input
{
	class CInputTranslator;
}

namespace UI
{
	class CUIServer;
}

namespace Frame
{
	typedef std::unique_ptr<class CView> PView;
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
typedef Ptr<class CApplicationState> PApplicationState;

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
	std::unique_ptr<Resources::CResourceManager> ResMgr;
	//std::unique_ptr<UI::CUIServer> UIServer;

	Data::PParams GlobalSettings;
	Data::PParams OverrideSettings; // From a command line
	CStrID CurrentUserID;
	std::vector<CUser> ActiveUsers;
	CString AppDataPath;

	PApplicationState CurrState;
	PApplicationState RequestedState;

	double BaseTime = 0.0;
	double PrevTime = 0.0;
	double FrameTime = 0.0;
	float TimeScale = 1.f;

	template<class T> T GetSetting(const char* pKey, const T& Default, CStrID UserID) const;
	template<class T> bool SetSetting(const char* pKey, const T& Value, CStrID UserID);

	DECLARE_EVENT_HANDLER(OnClosing, OnMainWindowClosing);

	void			RequestState(PApplicationState NewState);

public:

	CApplication(Sys::IPlatform& _Platform);
	virtual ~CApplication();

	Sys::IPlatform&	GetPlatform() const { return Platform; }

	IO::CIOServer&	IO() const;
	//UI::CUIServer&	UI() const;
	Resources::CResourceManager& ResourceManager() const;

	void			SetAppDataPath(const char* pPath) { AppDataPath = pPath; }
	CStrID			CreateUserProfile(const char* pUserID);
	bool			DeleteUserProfile(const char* pUserID);
	UPTR			EnumUserProfiles(CArray<CStrID>& Out) const;
	CStrID			ActivateUser(CStrID UserID);
	void			DeactivateUser(CStrID UserID);
	CStrID			GetCurrentUserID() const { return CurrentUserID; }

	Input::CInputTranslator* GetUserInput(CStrID UserID) const;

	void			ParseCommandLine(const char* pCmdLine);
	bool			LoadSettings(const char* pFilePath, bool Reload = false, CStrID UserID = CStrID::Empty); //???use stream?
	void			SaveSettings();
	bool			GetBoolSetting(const char* pKey, bool Default, CStrID UserID);
	int				GetIntSetting(const char* pKey, int Default, CStrID UserID);
	float			GetFloatSetting(const char* pKey, float Default, CStrID UserID);
	CString			GetStringSetting(const char* pKey, const CString& Default, CStrID UserID);
	bool			SetBoolSetting(const char* pKey, bool Value, CStrID UserID);
	bool			SetIntSetting(const char* pKey, int Value, CStrID UserID);
	bool			SetFloatSetting(const char* pKey, float Value, CStrID UserID);
	bool			SetStringSetting(const char* pKey, const CString& Value, CStrID UserID);

	bool			Run(PApplicationState InitialState);
	bool			Update();
	void			Term();

	//allow multiple instances

	void			ExitOnWindowClosed(Sys::COSWindow* pWindow);

	//???store windows inside app?
	int				CreateRenderWindow(Render::CGPUDriver& GPU, U32 Width, U32 Height);
	Frame::PView	CreateFrameView(Render::CGPUDriver& GPU, int SwapChainID, const char* pRenderPathID, bool WithGUI);
	//POSConsoleWindow CreateConsoleWindow();
};

}
};
