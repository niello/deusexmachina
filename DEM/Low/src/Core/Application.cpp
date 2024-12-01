#include "Application.h"

#include <Core/ApplicationState.h>
#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <IO/PathUtils.h>
#include <Events/EventServer.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Math/Math.h>
#include <System/Platform.h>
#include <System/SystemEvents.h>
#include <System/OSWindow.h>
#include <System/OSFileSystem.h>
#include <Frame/View.h>
#include <Render/SwapChain.h>
#include <Render/GPUDriver.h>
#include <Render/VideoDriverFactory.h>
#include <Data/ParamsUtils.h>
#include <Input/InputTranslator.h>
#include <regex>

// Scene bootstrapper includes
// TODO: consider incapsulating into methods of relevant subsystems
#include <Frame/GraphicsResourceManager.h>
#include <Frame/RenderPhaseGeometry.h>
#include <UI/RenderPhaseGUI.h>
#include <Debug/RenderPhaseDebugDraw.h>
//
#include <Render/Model.h>
#include <Render/ModelRenderer.h>
#include <Render/Skybox.h>
#include <Render/SkyboxRenderer.h>
#include <Render/Terrain.h>
#include <Render/TerrainRenderer.h>
//
#include <Frame/RenderPath.h>
#include <Frame/Renderables/ModelAttribute.h>
#include <Frame/Renderables/TerrainAttribute.h>
#include <Frame/Renderables/SkyboxAttribute.h>
#include <Frame/Lights/DirectionalLightAttribute.h>
#include <Frame/Lights/PointLightAttribute.h>
#include <Frame/Lights/SpotLightAttribute.h>
#include <Frame/Lights/IBLAmbientLightAttribute.h>
#include <Render/SkinInfo.h>
#include <Render/SkinInfoLoaderSKN.h>
#include <Render/MeshData.h>
#include <Render/MeshLoaderNVX2.h>
#include <Render/MeshLoaderMSH.h>
#include <Render/TextureData.h>
#include <Render/TextureLoaderTGA.h>
#include <Render/TextureLoaderDDS.h>
#include <Render/TextureLoaderCDLOD.h>
#include <Render/CDLODData.h>
#include <Render/CDLODDataLoader.h>
#include <Physics/CollisionLoaderCDLOD.h>
#include <Physics/CollisionLoaderHRD.h>
#include <Physics/CollisionAttribute.h>
#include <Animation/AnimationClip.h>
#include <Animation/AnimationLoaderANM.h>
#include <Physics/CollisionShape.h>
#include <Scene/SceneNode.h>
#include <Scene/SceneNodeLoaderSCN.h>
#include <Scripting/ScriptAsset.h>
#include <Scripting/ScriptAssetLoader.h>

namespace DEM::Core
{

CString GetProfilesPath(const CString& AppDataPath)
{
	return AppDataPath + "profiles/";
}
//---------------------------------------------------------------------

static CString GetUserProfilePath(const CString& AppDataPath, const char* pUserID)
{
	CString Path = GetProfilesPath(AppDataPath);
	Path += pUserID;
	PathUtils::EnsurePathHasEndingDirSeparator(Path);
	return Path;
}
//---------------------------------------------------------------------

static CString GetUserSavesPath(const CString& AppDataPath, const char* pUserID)
{
	return GetUserProfilePath(AppDataPath, pUserID) + "saves/";
}
//---------------------------------------------------------------------

static CString GetUserScreenshotsPath(const CString& AppDataPath, const char* pUserID)
{
	return GetUserProfilePath(AppDataPath, pUserID) + "screenshots/";
}
//---------------------------------------------------------------------

static CString GetUserCurrDataPath(const CString& AppDataPath, const char* pUserID)
{
	return GetUserProfilePath(AppDataPath, pUserID) + "current/";
}
//---------------------------------------------------------------------

static CString GetUserSettingsFilePath(const CString& AppDataPath, const char* pUserID)
{
	return GetUserProfilePath(AppDataPath, pUserID) + "Settings.hrd";
}
//---------------------------------------------------------------------

//???empty constructor, add Init to process init-time failures?
CApplication::CApplication(Sys::IPlatform& _Platform)
	: Platform(_Platform)
#ifdef TRACY_ENABLE
		// Leave one core for tracy worker and one for Tracy.exe
	, _JobSystem({ DEM::Jobs::CWorkerConfig::Normal(std::max(2u, std::thread::hardware_concurrency()) - 2), DEM::Jobs::CWorkerConfig::Sleepy(4) })
#else
	, _JobSystem({ DEM::Jobs::CWorkerConfig::Default(), DEM::Jobs::CWorkerConfig::Sleepy(4) })
#endif
{
	// check multiple instances

	// create default file system from platform
	// setup hard assigns from platform and application

	n_new(::Events::CEventServer);
	IOServer.reset(n_new(IO::CIOServer));
	ResMgr.reset(n_new(Resources::CResourceManager(IOServer.get())));

	// Initialize unclaimed input translator. It is used to translate input from
	// devices not yet tied to a specific user. Typically only UI receives that input.
	UnclaimedInput = std::move(CreateInput(CStrID::Empty));

	// Attach all existing input devices to the unclaimed context
	CArray<Input::PInputDevice> InputDevices;
	Platform.EnumInputDevices(InputDevices);
	for (auto& Device : InputDevices)
		UnclaimedInput->ConnectToDevice(Device.Get());

	DISP_SUBSCRIBE_NEVENT(&_Platform, InputDeviceArrived, CApplication, OnInputDeviceArrived);
	DISP_SUBSCRIBE_NEVENT(&_Platform, InputDeviceRemoved, CApplication, OnInputDeviceRemoved);
}
//---------------------------------------------------------------------

CApplication::~CApplication()
{
	Term();
	if (::Events::CEventServer::HasInstance()) n_delete(EventSrv);
}
//---------------------------------------------------------------------

IO::CIOServer& CApplication::IO() const
{
	return *IOServer;
}
//---------------------------------------------------------------------

Resources::CResourceManager& CApplication::ResourceManager() const
{
	return *ResMgr;
}
//---------------------------------------------------------------------

bool CApplication::IsValidUserProfileName(const char* pUserID, UPTR MaxLength) const
{
	if (!pUserID || !*pUserID || !MaxLength) FAIL;

	std::string UserStr(pUserID);

	std::smatch smatch;
	std::regex regex("^[\\w .\\-+()]{1," + std::to_string(MaxLength) + "}$");
	if (!std::regex_match(UserStr, smatch, regex) || smatch.empty()) FAIL;

	// TODO: checking WritablePath FS is more correct, it can be implemented
	// to support file names not supported by a native FS.
	if (!Platform.GetFileSystemInterface()->IsValidFileName(pUserID)) FAIL;

	if (UserProfileExists(pUserID)) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CApplication::UserProfileExists(const char* pUserID) const
{
	//???must be case-insensitive regardless of FS?
	const CString Path = GetUserProfilePath(WritablePath, pUserID);
	return IO().DirectoryExists(Path);
}
//---------------------------------------------------------------------

CStrID CApplication::CreateUserProfile(const char* pUserID, bool Overwrite)
{
	if (!IsValidUserProfileName(pUserID)) return CStrID::Empty;

	CString UserID(pUserID);
	UserID.Trim();

	if (UserProfileExists(pUserID))
	{
		if (!Overwrite || !DeleteUserProfile(pUserID)) return CStrID::Empty;
	}

	const CString Path = GetUserProfilePath(WritablePath, pUserID);
	if (!IO().CreateDirectory(Path)) return CStrID::Empty;

	bool Result = true;

	if (Result && !IO().CopyFile(UserSettingsTemplate, Path + "Settings.hrd")) Result = false;
	if (Result && !IO().CreateDirectory(Path + "saves")) Result = false;
	if (Result && !IO().CreateDirectory(Path + "screenshots")) Result = false;
	if (Result && !IO().CreateDirectory(Path + "current")) Result = false;

	if (!Result)
	{
		IO().DeleteDirectory(Path);
		return CStrID::Empty;
	}

	return CStrID(UserID);
}
//---------------------------------------------------------------------

bool CApplication::DeleteUserProfile(const char* pUserID)
{
	// Can't delete current user
	if (CurrentUserID == pUserID) FAIL;

	//???or deactivate?
	if (IsUserActive(CStrID(pUserID))) FAIL;

	return IO().DeleteDirectory(GetUserProfilePath(WritablePath, pUserID));
}
//---------------------------------------------------------------------

UPTR CApplication::EnumUserProfiles(CArray<CStrID>& Out) const
{
	CString ProfilesDir = GetProfilesPath(WritablePath);
	if (!IO().DirectoryExists(ProfilesDir)) return 0;

	const UPTR OldCount = Out.GetCount();

	IO::CFSBrowser Browser;
	Browser.SetAbsolutePath(ProfilesDir);
	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryDir())
			Out.Add(CStrID(Browser.GetCurrEntryName()));
	}
	while (Browser.NextCurrDirEntry());

	return Out.GetCount() - OldCount;
}
//---------------------------------------------------------------------

CStrID CApplication::ActivateUser(CStrID UserID, Input::PInputTranslator&& Input)
{
	if (!UserID.IsValid()) return CStrID::Empty;

	auto It = std::find_if(ActiveUsers.cbegin(), ActiveUsers.cend(), [UserID](const CUser& User)
	{
		return User.ID == UserID;
	});
	if (It != ActiveUsers.cend()) return UserID;

	CString Path = GetUserProfilePath(WritablePath, UserID.CStr());
	PathUtils::EnsurePathHasEndingDirSeparator(Path);

	if (!IO().DirectoryExists(Path)) return CStrID::Empty;

	{
		CUser NewUser;
		NewUser.ID = UserID;

		// OnSettingsLoaded is not sent here, subscribe on OnUserActivated
		NewUser.Settings = ParamsUtils::LoadParamsFromHRD(GetUserSettingsFilePath(WritablePath, UserID.CStr()));
		if (!NewUser.Settings) return CStrID::Empty;

		ActiveUsers.push_back(std::move(NewUser));
	}

	// Load input params after activating the user
	if (Input)
	{
		ActiveUsers.back().Input = std::move(Input);
		ActiveUsers.back().Input->SetUserID(UserID);
	}
	else
	{
		ActiveUsers.back().Input = std::move(CreateInput(UserID));
	}

	Data::PParams Params = n_new(Data::CParams(1));
	Params->Set(CStrID("UserID"), UserID);
	EventSrv->FireEvent(CStrID("OnUserActivated"), Params);

	//???need current user at all? maybe end-application must handle it?
	// Make an activated user current if there was no current user
	if (!CurrentUserID.IsValid())
	{
		// All unclaimed input is assigned to the first user
		UnclaimedInput->TransferAllDevices(ActiveUsers.back().Input.get());

		CurrentUserID = UserID;

		Data::PParams Params = n_new(Data::CParams(2));
		Params->Set(CStrID("Old"), CStrID::Empty);
		Params->Set(CStrID("New"), CurrentUserID);
		EventSrv->FireEvent(CStrID("OnCurrentUserChanged"), Params);
	}

	return UserID;
}
//---------------------------------------------------------------------

void CApplication::DeactivateUser(CStrID UserID)
{
	auto It = std::find_if(ActiveUsers.begin(), ActiveUsers.end(), [UserID](const CUser& User) { return User.ID == UserID; });
	if (It == ActiveUsers.end()) return;

	if (It->SettingsChanged && It->Settings)
		ParamsUtils::SaveParamsToHRD(GetUserSettingsFilePath(WritablePath, UserID.CStr()), *It->Settings);

	// All user input is returned to unclaimed
	It->Input->TransferAllDevices(UnclaimedInput.get());

	if (CurrentUserID == UserID)
	{
		auto NewUserIt = std::find_if(ActiveUsers.begin(), ActiveUsers.end(), [UserID](const CUser& User) { return User.ID != UserID; });

		CurrentUserID = (NewUserIt == ActiveUsers.end()) ? CStrID::Empty : NewUserIt->ID;

		Data::PParams Params = n_new(Data::CParams(2));
		Params->Set(CStrID("Old"), UserID);
		Params->Set(CStrID("New"), CurrentUserID);
		EventSrv->FireEvent(CStrID("OnCurrentUserChanged"), Params);
	}

	ActiveUsers.erase(It);

	Data::PParams Params = n_new(Data::CParams(1));
	Params->Set(CStrID("UserID"), UserID);
	EventSrv->FireEvent(CStrID("OnUserDeactivated"), Params);
}
//---------------------------------------------------------------------

bool CApplication::IsUserActive(CStrID UserID) const
{
	auto It = std::find_if(ActiveUsers.cbegin(), ActiveUsers.cend(), [UserID](const CUser& User) { return User.ID == UserID; });
	return It != ActiveUsers.cend();
}
//---------------------------------------------------------------------

Input::CInputTranslator* CApplication::GetUserInput(CStrID UserID) const
{
	auto It = std::find_if(ActiveUsers.cbegin(), ActiveUsers.cend(), [UserID](const CUser& User) { return User.ID == UserID; });
	return It == ActiveUsers.cend() ? nullptr : It->Input.get();
}
//---------------------------------------------------------------------

Input::CInputTranslator* CApplication::GetUnclaimedInput() const
{
	return UnclaimedInput.get();
}
//---------------------------------------------------------------------

Input::PInputTranslator CApplication::CreateInput(CStrID UserID) const
{
	Input::PInputTranslator NewInput(n_new(Input::CInputTranslator(UserID)));

	// Initialize a bypass context for UI
	const CStrID sidUIBypass("UIBypass");
	NewInput->CreateContext(sidUIBypass, true);
	NewInput->EnableContext(sidUIBypass);

	if (InputDesc)
	{
		NewInput->LoadSettings(*InputDesc);
		NewInput->UpdateParams(*this);
	}

	return NewInput;
}
//---------------------------------------------------------------------

void CApplication::ParseCommandLine(const char* pCmdLine)
{
	if (!pCmdLine || !*pCmdLine) return;

	// TODO: implement through CLI11
	NOT_IMPLEMENTED;

	//!!!DBG TMP!
	if (!strcmp(pCmdLine, "-O TestFloat=999.0"))
	{
		OverrideSettings = n_new(Data::CParams(1));
		OverrideSettings->Set<float>(CStrID("TestFloat"), 999.f);

		Data::PParams Params = n_new(Data::CParams(2));
		Params->Set(CStrID("UserID"), CStrID::Empty);
		Params->Set(CStrID("Key"), CStrID("TestFloat"));
		EventSrv->FireEvent(CStrID("OnSettingChanged"), Params);
	}
}
//---------------------------------------------------------------------

bool CApplication::LoadGlobalSettings(const char* pFilePath)
{
	GlobalSettingsPath = pFilePath;

	Data::PParams NewSettings = ParamsUtils::LoadParamsFromHRD(pFilePath);
	if (!NewSettings) FAIL;

	GlobalSettings = NewSettings;
	GlobalSettingsChanged = false;

	Data::PParams Params = n_new(Data::CParams(1));
	Params->Set(CStrID("UserID"), CStrID::Empty);
	EventSrv->FireEvent(CStrID("OnSettingsLoaded"), Params);

	OK;
}
//---------------------------------------------------------------------

void CApplication::SaveSettings()
{
	// Save global settings if necessary
	if (GlobalSettingsChanged && GlobalSettings)
	{
		if (GlobalSettingsPath.IsEmpty() ||
			IO().IsFileReadOnly(GlobalSettingsPath) ||
			!ParamsUtils::SaveParamsToHRD(GlobalSettingsPath, *GlobalSettings))
		{
			::Sys::Error("CApplication::SaveSettings() > failed to save global settings\n");
		}
		else
			GlobalSettingsChanged = false;
	}

	for (auto& User : ActiveUsers)
	{
		if (User.SettingsChanged && User.Settings)
		{
			const auto UserSettingsPath = GetUserSettingsFilePath(WritablePath, User.ID.CStr());

			if (UserSettingsPath.IsEmpty() ||
				IO().IsFileReadOnly(UserSettingsPath) ||
				!ParamsUtils::SaveParamsToHRD(UserSettingsPath, *User.Settings))
			{
				::Sys::Error("CApplication::SaveSettings() > failed to save user settings\n");
			}
			else
				User.SettingsChanged = false;
		}
	}
}
//---------------------------------------------------------------------

Data::CData* CApplication::FindSetting(const char* pKey, CStrID UserID) const
{
	CStrID Key(pKey);
	Data::CData* pData = nullptr;

	if (OverrideSettings && OverrideSettings->TryGet(pData, Key))
		return pData;

	if (UserID.IsValid())
	{
		auto It = std::find_if(ActiveUsers.cbegin(), ActiveUsers.cend(), [UserID](const CUser& User) { return User.ID == UserID; });
		if (It == ActiveUsers.cend())
		{
			::Sys::Error("CApplication::GetSetting() > requested user is inactive");
			return nullptr;
		}

		if (It->Settings && It->Settings->TryGet(pData, Key))
			return pData;
	}

	if (GlobalSettings && GlobalSettings->TryGet(pData, Key))
		return pData;

	return nullptr;
}
//---------------------------------------------------------------------

// For internal use only. Made private to avoid declaring this in a header.
template<class T>
T CApplication::GetSetting(const char* pKey, const T& Default, CStrID UserID) const
{
	const Data::CData* pData = FindSetting(pKey, UserID);
	return (pData && pData->IsA<T>()) ? pData->GetValue<T>() : Default;
}
//---------------------------------------------------------------------

bool CApplication::GetBoolSetting(const char* pKey, bool Default, CStrID UserID) const
{
	return GetSetting<bool>(pKey, Default, UserID);
}
//---------------------------------------------------------------------

int CApplication::GetIntSetting(const char* pKey, int Default, CStrID UserID) const
{
	return GetSetting<int>(pKey, Default, UserID);
}
//---------------------------------------------------------------------

float CApplication::GetFloatSetting(const char* pKey, float Default, CStrID UserID) const
{
	return GetSetting<float>(pKey, Default, UserID);
}
//---------------------------------------------------------------------

CString CApplication::GetStringSetting(const char* pKey, const CString& Default, CStrID UserID) const
{
	return GetSetting<CString>(pKey, Default, UserID);
}
//---------------------------------------------------------------------

bool CApplication::SetSetting(const char* pKey, const Data::CData& Value, CStrID UserID)
{
	CStrID Key(pKey);
	Data::CParams* pSettings = nullptr;

	auto It = ActiveUsers.end();
	if (UserID.IsValid())
	{
		It = std::find_if(ActiveUsers.begin(), ActiveUsers.end(), [UserID](const CUser& User) { return User.ID == UserID; });
		if (It == ActiveUsers.end())
		{
			::Sys::Error("CApplication::SetSetting() > requested user is inactive");
			FAIL;
		}

		if (!It->Settings) It->Settings = n_new(Data::CParams);
		pSettings = It->Settings.Get();
	}
	else
	{
		if (!GlobalSettings) GlobalSettings = n_new(Data::CParams);
		pSettings = GlobalSettings.Get();
	}

	Data::CParam* pPrm = pSettings->Find(Key);

	// Value not changed
	if (pPrm && pPrm->GetRawValue() == Value) FAIL;

	if (pPrm) pPrm->SetValue(Value);
	else pSettings->Set(Key, Value);

	if (UserID.IsValid())
		It->SettingsChanged = true;
	else
		GlobalSettingsChanged = true;

	Data::PParams Params = n_new(Data::CParams(2));
	Params->Set(CStrID("UserID"), UserID);
	Params->Set(CStrID("Key"), Key);
	EventSrv->FireEvent(CStrID("OnSettingChanged"), Params);

	OK;
}
//---------------------------------------------------------------------

bool CApplication::SetBoolSetting(const char* pKey, bool Value, CStrID UserID)
{
	return SetSetting(pKey, Value, UserID);
}
//---------------------------------------------------------------------

bool CApplication::SetIntSetting(const char* pKey, int Value, CStrID UserID)
{
	return SetSetting(pKey, Value, UserID);
}
//---------------------------------------------------------------------

bool CApplication::SetFloatSetting(const char* pKey, float Value, CStrID UserID)
{
	return SetSetting(pKey, Value, UserID);
}
//---------------------------------------------------------------------

bool CApplication::SetStringSetting(const char* pKey, const CString& Value, CStrID UserID)
{
	return SetSetting(pKey, Value, UserID);
}
//---------------------------------------------------------------------

// Creates a GUI window most suitable for 3D scene rendering, based on app & profile settings
int CApplication::CreateRenderWindow(Render::CGPUDriver& GPU, const Data::CRect& Rect)
{
	auto Wnd = Platform.CreateGUIWindow();
	Wnd->SetRect(Rect);

	Render::CRenderTargetDesc BBDesc;
	BBDesc.Format = Render::PixelFmt_DefaultBackBuffer;
	BBDesc.MSAAQuality = Render::MSAA_None;
	BBDesc.UseAsShaderInput = false;
	BBDesc.MipLevels = 0;
	BBDesc.Width = 0;
	BBDesc.Height = 0;

	Render::CSwapChainDesc SCDesc;
	SCDesc.BackBufferCount = 2;
	SCDesc.SwapMode = Render::SwapMode_CopyDiscard;
	SCDesc.Flags = Render::SwapChain_VSync;

	const int SwapChainID = GPU.CreateSwapChain(BBDesc, SCDesc, Wnd);
	n_assert(GPU.SwapChainExists(SwapChainID));
	return SwapChainID;
}
//---------------------------------------------------------------------

// Initializes 3D graphics, resources etc and makes an application ready to work with 3D scenes.
// This function is intended for fast typical setup and may be completely replaced with an application
// code if more sophisticated initialization is required. Don't call more than once!
Frame::PGraphicsResourceManager CApplication::BootstrapGraphics(Render::PVideoDriverFactory Gfx)
{
	// Register render path classes in the factory

	Frame::CRenderPhaseGUI::ForceFactoryRegistration();
	Frame::CRenderPhaseGeometry::ForceFactoryRegistration();
	Frame::CRenderPhaseDebugDraw::ForceFactoryRegistration();
	Frame::CModelAttribute::ForceFactoryRegistration();
	Frame::CTerrainAttribute::ForceFactoryRegistration();
	Frame::CSkyboxAttribute::ForceFactoryRegistration();
	Frame::CIBLAmbientLightAttribute::ForceFactoryRegistration();
	Physics::CCollisionAttribute::ForceFactoryRegistration();
	Render::CModel::ForceFactoryRegistration();
	Render::CModelRenderer::ForceFactoryRegistration();
	Render::CSkybox::ForceFactoryRegistration();
	Render::CSkyboxRenderer::ForceFactoryRegistration();
	Render::CTerrain::ForceFactoryRegistration();
	Render::CTerrainRenderer::ForceFactoryRegistration();

	// Initialize the default graphics device

	if (!Gfx->Create())
	{
		::Sys::Error("Can't create video driver factory");
		return nullptr;
	}

	Render::PGPUDriver GPU = Gfx->CreateGPUDriver(Render::Adapter_Primary, Render::GPU_Hardware);
	if (!GPU)
	{
		::Sys::Error("Can't create GPU driver");
		return nullptr;
	}

	// Register resource loaders

	ResMgr->RegisterDefaultCreator("scn", &Scene::CSceneNode::RTTI, n_new(Resources::CSceneNodeLoaderSCN(*ResMgr)));
	ResMgr->RegisterDefaultCreator("anm", &DEM::Anim::CAnimationClip::RTTI, n_new(Resources::CAnimationLoaderANM(*ResMgr)));
	ResMgr->RegisterDefaultCreator("cdlod", &Render::CCDLODData::RTTI, n_new(Resources::CCDLODDataLoader(*ResMgr)));
	ResMgr->RegisterDefaultCreator("cdlod", &Render::CTextureData::RTTI, n_new(Resources::CTextureLoaderCDLOD(*ResMgr)));
	ResMgr->RegisterDefaultCreator("cdlod", &Physics::CCollisionShape::RTTI, n_new(Resources::CCollisionLoaderCDLOD(*ResMgr)));
	ResMgr->RegisterDefaultCreator("hrd", &Physics::CCollisionShape::RTTI, n_new(Resources::CCollisionLoaderHRD(*ResMgr)));
	ResMgr->RegisterDefaultCreator("nvx2", &Render::CMeshData::RTTI, n_new(Resources::CMeshLoaderNVX2(*ResMgr)));
	ResMgr->RegisterDefaultCreator("msh", &Render::CMeshData::RTTI, n_new(Resources::CMeshLoaderMSH(*ResMgr)));
	ResMgr->RegisterDefaultCreator("skn", &Render::CSkinInfo::RTTI, n_new(Resources::CSkinInfoLoaderSKN(*ResMgr)));
	ResMgr->RegisterDefaultCreator("dds", &Render::CTextureData::RTTI, n_new(Resources::CTextureLoaderDDS(*ResMgr)));
	ResMgr->RegisterDefaultCreator("tga", &Render::CTextureData::RTTI, n_new(Resources::CTextureLoaderTGA(*ResMgr)));
	ResMgr->RegisterDefaultCreator("lua", &DEM::Scripting::CScriptAsset::RTTI, n_new(Resources::CScriptAssetLoader(*ResMgr)));

	// Return graphics resource manager object, which allows to create frame views

	return n_new(Frame::CGraphicsResourceManager(*ResMgr, *GPU, &_JobSystem));
}
//---------------------------------------------------------------------

bool CApplication::Run(PApplicationState InitialState)
{
	BaseTime = Platform.GetSystemTime();
	PrevTime = BaseTime;
	FrameTime = 0.0;

	if (InitialState && (&InitialState->GetApplication()) != this)
	{
		::Sys::Error("CApplication::Run() > state is not attached to this application");
		FAIL;
	}

	_RequestedState = InitialState;

	return InitialState.IsValidPtr();
}
//---------------------------------------------------------------------

bool CApplication::Update()
{
	ZoneScoped;

	// Update time

	constexpr double MAX_FRAME_TIME = 0.25;

	const double CurrTime = Platform.GetSystemTime();
	FrameTime = CurrTime - PrevTime;
	if (FrameTime < 0.0) FrameTime = 1.0 / 60.0;
	else if (FrameTime > MAX_FRAME_TIME) FrameTime = MAX_FRAME_TIME;
	FrameTime *= TimeScale;

	PrevTime = CurrTime;

	// Dump setting changes

	SaveSettings();

	// Process OS messages etc

	if (!Platform.Update()) return false;

	// Update application state

	while (_CurrState != _RequestedState)
	{
		// Remember requested state for the case it is changed in OnExit or OnEnter
		// NB: don't abuse it, because it can lead to infinite loop here!
		PApplicationState RequestedStateCopy = _RequestedState;

		if (_CurrState) _CurrState->OnExit(RequestedStateCopy.Get());
		if (RequestedStateCopy) RequestedStateCopy->OnEnter(_CurrState.Get());
		_CurrState = RequestedStateCopy;
	}

	if (!_CurrState) return false;

	_RequestedState = _CurrState->Update(FrameTime);

	// Tracy main frame mark
	FrameMark;

	return true;
}
//---------------------------------------------------------------------

//!!!Init()'s pair!
void CApplication::Term()
{
	SaveSettings();

	UNSUBSCRIBE_EVENT(InputDeviceArrived);
	UNSUBSCRIBE_EVENT(InputDeviceRemoved);
	UNSUBSCRIBE_EVENT(OnClosing);

	//!!!kill all windows!
}
//---------------------------------------------------------------------

void CApplication::RequestState(PApplicationState NewState)
{
	n_assert(!NewState || &NewState->GetApplication() == this);
	_RequestedState = NewState;
}
//---------------------------------------------------------------------

CApplicationState* CApplication::GetCurrentState() const
{
	return _CurrState.Get();
}
//---------------------------------------------------------------------

void CApplication::ExitOnWindowClosed(Sys::COSWindow* pWindow)
{
	if (pWindow)
	{
		DISP_SUBSCRIBE_PEVENT(pWindow, OnClosing, CApplication, OnMainWindowClosing);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnClosing);
	}
}
//---------------------------------------------------------------------

bool CApplication::OnMainWindowClosing(::Events::CEventDispatcher* pDispatcher, const ::Events::CEventBase& Event)
{
	UNSUBSCRIBE_EVENT(OnClosing);
	RequestState(nullptr);
	OK;
}
//---------------------------------------------------------------------

bool CApplication::OnInputDeviceArrived(::Events::CEventDispatcher* pDispatcher, const ::Events::CEventBase& Event)
{
	const Event::InputDeviceArrived& Ev = static_cast<const Event::InputDeviceArrived&>(Event);

	// All new unclaimed input is assigned to the current user
	// FIXME: is per-application?
	if (Ev.FirstSeen)
	{
		if (CurrentUserID.IsValid())
			GetUserInput(CurrentUserID)->ConnectToDevice(Ev.Device.Get());
		else
			UnclaimedInput->ConnectToDevice(Ev.Device.Get());
	}

	// Probably more correct way:
	/*
	// When a new input device is connected, it becomes an unclaimed input source.
	// It can be assigned to an user later.
	if (Ev.FirstSeen)
		UnclaimedInput->ConnectToDevice(Ev.Device.Get());

	and then forward this event to client
	*/

	OK;
}
//---------------------------------------------------------------------

bool CApplication::OnInputDeviceRemoved(::Events::CEventDispatcher* pDispatcher, const ::Events::CEventBase& Event)
{
	const Event::InputDeviceRemoved& Ev = static_cast<const Event::InputDeviceRemoved&>(Event);

	// FIXME: remove this method if no action required

	OK;
}
//---------------------------------------------------------------------

}
