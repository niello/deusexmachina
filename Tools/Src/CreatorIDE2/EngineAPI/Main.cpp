#include "App/CIDEApp.h"
#include <App/Environment.h>
#include "FactoryRegHelper.h"
#include <Game/EntityManager.h>

using namespace App;

bool Initialized = false;
bool FactoryRegistrationRequired = true;

API void* CreateEngine()
{
	n_assert(!AppInst);
	if (FactoryRegistrationRequired)
	{
		ForceFactoryRegistration();
		FactoryRegistrationRequired = false;
	}
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
		//LoaderSrv->LoadEmptyLevel();
		//EntityMgr->AttachEntity(CIDEApp->EditorCamera);
		//FocusMgr->SetFocusEntity(CIDEApp->EditorCamera);
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
	//LoaderSrv->CommitChangesToDB();
	//!!!restore current entity here if transform or at least check why this property isn't active!
	//LoaderSrv->UnloadLevel();
	CIDEApp->Close();
	delete CIDEApp;
	AppInst = NULL;
}
//---------------------------------------------------------------------

API void GetDLLName(CIDEAppHandle Handle, char* Name)
{
	DeclareCIDEApp(Handle);
	sprintf_s(Name, 255, (Initialized) ? CIDEApp->GetAppName().CStr() : "Not initialized!");
}
//---------------------------------------------------------------------

API void GetDLLVersion(CIDEAppHandle Handle, char* Version)
{
	DeclareCIDEApp(Handle);
	sprintf_s(Version, 255, (Initialized) ? CIDEApp->GetAppVersion().CStr() : "Not initialized!");
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

API void SetDataPathCallback(CIDEAppHandle Handle, IO::CDataPathCallback Cb, IO::CReleaseMemoryCallback ReleaseCb)
{
	DeclareCIDEApp(Handle);
	CIDEApp->SetDataPathCB(Cb, ReleaseCb);
}
//---------------------------------------------------------------------
