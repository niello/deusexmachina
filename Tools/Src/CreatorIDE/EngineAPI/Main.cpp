#include <App/CIDEApp.h>
#include <App/Environment.h>
#include <FactoryRegHelper.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/FocusManager.h>

App::CCIDEApp AppInst;
bool Initialized = false;

API int Init(HWND ParentWnd, LPCSTR ProjDir)
{
	AppInst.SetParentWindow(ParentWnd);
	AppEnv->SetProjectDirectory(ProjDir);
	if (AppInst.Open()) 
	{
		LoaderSrv->LoadEmptyLevel();
		EntityMgr->AttachEntity(AppInst.EditorCamera);
		FocusMgr->SetFocusEntity(AppInst.EditorCamera);
		//!!!error but should not be! LoaderSrv->LoadLevel("Eger_Cathedral_Courtyard");
		//LoaderSrv->NewGame();
		Initialized = true;
		return 0;
	}
	return 1;
}
//---------------------------------------------------------------------

API bool Advance()
{
	return AppInst.AdvanceFrame();
}
//---------------------------------------------------------------------

API void Release()
{
	Initialized = false;
	AppInst.UnloadLevel(true);
	AppInst.Close();
}
//---------------------------------------------------------------------

API void GetDLLName(char* Name)
{
	sprintf_s(Name, 255, (Initialized) ? CIDEApp->GetAppName().Get() : "Not initialized!");
}
//---------------------------------------------------------------------

API void GetDLLVersion(char* Version)
{
	sprintf_s(Version, 255, (Initialized) ? CIDEApp->GetAppVersion().Get() : "Not initialized!");
}
//---------------------------------------------------------------------

API int GetDLLVersionCode()
{
	return DEM_API_VERSION;
}
//---------------------------------------------------------------------

API bool SetEditorTool(LPCSTR Name)
{
	return AppInst.SetEditorTool(Name);
}
//---------------------------------------------------------------------

API bool SelectEntity(const char* UID)
{
	if (UID) return AppInst.SelectEntity(EntityMgr->GetEntityByID(CStrID(UID)));
	else AppInst.ClearSelectedEntities();
	OK;
}
//---------------------------------------------------------------------
