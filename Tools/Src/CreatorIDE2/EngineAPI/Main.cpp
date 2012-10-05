#include <App/CIDEApp.h>
#include <App/Environment.h>
#include <FactoryRegHelper.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/FocusManager.h>

using namespace App;

bool Initialized = false;

API void* CreateEngine()
{
	n_assert(!AppInst);
	AppInst = new CCIDEApp();
	return (void*)AppInst;
}
//---------------------------------------------------------------------

API int Init(CIDEAppHandle Handle, HWND ParentWnd, LPCSTR ProjDir)
{
	DeclareCIDEApp(Handle);
	CIDEApp->SetParentWindow(ParentWnd);
	AppEnv->SetProjectDirectory(ProjDir);
	if (CIDEApp->Open()) 
	{
		LoaderSrv->LoadEmptyLevel();
		EntityMgr->AttachEntity(CIDEApp->EditorCamera);
		FocusMgr->SetFocusEntity(CIDEApp->EditorCamera);
		//!!!error but should not be! LoaderSrv->LoadLevel("Eger_Cathedral_Courtyard");
		//LoaderSrv->NewGame();
		Initialized = true;
		return 0;
	}
	return 1;
}
//---------------------------------------------------------------------

API bool Advance(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	return CIDEApp->AdvanceFrame();
}
//---------------------------------------------------------------------

API void Release(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	Initialized = false;
	LoaderSrv->CommitChangesToDB();
	//!!!restore current entity here if transform or at least check why this property isn't active!
	LoaderSrv->UnloadLevel();
	CIDEApp->Close();
	delete CIDEApp;
	AppInst = NULL;
}
//---------------------------------------------------------------------

API void GetDLLName(CIDEAppHandle Handle, char* Name)
{
	DeclareCIDEApp(Handle);
	sprintf_s(Name, 255, (Initialized) ? CIDEApp->GetAppName().Get() : "Not initialized!");
}
//---------------------------------------------------------------------

API void GetDLLVersion(CIDEAppHandle Handle, char* Version)
{
	DeclareCIDEApp(Handle);
	sprintf_s(Version, 255, (Initialized) ? CIDEApp->GetAppVersion().Get() : "Not initialized!");
}
//---------------------------------------------------------------------

API int GetDLLVersionCode()
{
	return DEM_API_VERSION;
}
//---------------------------------------------------------------------

API void SetMouseButtonCallback(CIDEAppHandle Handle, CMouseButtonCallback Cb)
{
	DeclareCIDEApp(Handle);
	CIDEApp->MouseCB = Cb;
}
//---------------------------------------------------------------------
