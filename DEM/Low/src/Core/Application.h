#pragma once
#include <Data/Ptr.h>
#include <Data/Params.h>
#include <Data/Regions.h>
#include <Events/EventsFwd.h>
#include <Input/InputTranslator.h>
#include <Jobs/JobSystem.h>

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
	typedef Ptr<class CVideoDriverFactory> PVideoDriverFactory;
	typedef Ptr<class CGPUDriver> PGPUDriver;
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
	typedef std::unique_ptr<class CInputTranslator> PInputTranslator;
}

namespace Frame
{
	typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
}

namespace DEM::Sys
{
	class IPlatform;
	typedef Ptr<class COSWindow> POSWindow;
}

namespace DEM::Core
{
typedef Ptr<class CApplicationState> PApplicationState;

class CApplication
{
protected:

	struct CUser
	{
		CStrID ID;
		Input::PInputTranslator Input;
		Data::PParams Settings;
		bool SettingsChanged = false;
	};

	Sys::IPlatform& Platform; //???use unique ptr and heap-allocated platform?
	Jobs::CJobSystem _JobSystem;

	std::unique_ptr<IO::CIOServer> IOServer; //???rename to IOService?
	std::unique_ptr<Resources::CResourceManager> ResMgr;

	CString WritablePath;
	CString UserSettingsTemplate;
	CString GlobalSettingsPath;

	Data::PParams GlobalSettings;
	Data::PParams OverrideSettings; // From a command line
	bool GlobalSettingsChanged = false;
	CStrID CurrentUserID;
	std::vector<CUser> ActiveUsers;
	Input::PInputTranslator UnclaimedInput;
	Data::PParams InputDesc;

	PApplicationState _CurrState;
	PApplicationState _RequestedState;

	double BaseTime = 0.0;
	double PrevTime = 0.0;
	double FrameTime = 0.0;
	float TimeScale = 1.f;

	void SaveSettings();
	template<class T> T GetSetting(const char* pKey, const T& Default, CStrID UserID) const;

	DECLARE_EVENT_HANDLER(OnClosing, OnMainWindowClosing);
	DECLARE_EVENT_HANDLER(InputDeviceArrived, OnInputDeviceArrived);
	DECLARE_EVENT_HANDLER(InputDeviceRemoved, OnInputDeviceRemoved);

public:

	CApplication(Sys::IPlatform& _Platform);
	virtual ~CApplication();

	Sys::IPlatform&	GetPlatform() const { return Platform; }
	Jobs::CJobSystem& JobSystem() { return _JobSystem; }

	IO::CIOServer& IO() const;
	Resources::CResourceManager& ResourceManager() const;

	void				SetWritablePath(const char* pPath) { WritablePath = pPath; }
	void				SetUserSettingsTemplate(const char* pFilePath) { UserSettingsTemplate = pFilePath; }
	void				SetInputTranslationDesc(Data::PParams Desc) { InputDesc = Desc; }
	bool				IsValidUserProfileName(const char* pUserID, UPTR MaxLength = 40) const;
	bool				UserProfileExists(const char* pUserID) const;
	CStrID				CreateUserProfile(const char* pUserID, bool Overwrite = false);
	bool				DeleteUserProfile(const char* pUserID);
	UPTR				EnumUserProfiles(CArray<CStrID>& Out) const;
	CStrID				ActivateUser(CStrID UserID, Input::PInputTranslator&& Input = Input::PInputTranslator());
	void				DeactivateUser(CStrID UserID);
	bool				IsUserActive(CStrID UserID) const;
	CStrID				GetCurrentUserID() const { return CurrentUserID; }

	Input::CInputTranslator* GetUserInput(CStrID UserID) const;
	Input::CInputTranslator* GetUnclaimedInput() const;
	Input::PInputTranslator  CreateInput(CStrID UserID) const;

	void				ParseCommandLine(const char* pCmdLine);
	bool				LoadGlobalSettings(const char* pFilePath);
	Data::CData*		FindSetting(const char* pKey, CStrID UserID) const;
	bool				GetBoolSetting(const char* pKey, bool Default, CStrID UserID) const;
	int					GetIntSetting(const char* pKey, int Default, CStrID UserID) const;
	float				GetFloatSetting(const char* pKey, float Default, CStrID UserID) const;
	CString				GetStringSetting(const char* pKey, const CString& Default, CStrID UserID) const;
	bool				SetSetting(const char* pKey, const Data::CData& Value, CStrID UserID);
	bool				SetBoolSetting(const char* pKey, bool Value, CStrID UserID);
	bool				SetIntSetting(const char* pKey, int Value, CStrID UserID);
	bool				SetFloatSetting(const char* pKey, float Value, CStrID UserID);
	bool				SetStringSetting(const char* pKey, const CString& Value, CStrID UserID);

	bool				Run(PApplicationState InitialState);
	bool				Update();
	void				Term();
	void				RequestState(PApplicationState NewState);
	CApplicationState*	GetCurrentState() const;

	//allow multiple instances

	void				ExitOnWindowClosed(Sys::COSWindow* pWindow);

	//???store windows inside app?
	int					CreateRenderWindow(Render::CGPUDriver& GPU, const Data::CRect& Rect);
	//POSConsoleWindow CreateConsoleWindow();

	// Quickstart methods
	Frame::PGraphicsResourceManager BootstrapGraphics(Render::PVideoDriverFactory Gfx);
};

}
