#pragma once
#ifndef __CIDE_APP_H__
#define __CIDE_APP_H__

#include <App/AppFSM.h>
#include <Story/Dlg/DlgSystem.h>
#include <Story/Quests/QuestSystem.h>
#include <Items/ItemManager.h>
#include <Game/Entity.h>
#include <Data/Singleton.h>
#include <Data/DataServer.h>
#include "../StdAPI.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetClassName
#undef CopyFile
#undef CreateDirectory

enum EMouseAction
{
    Down = 0,
    Click,
    Up,
    DoubleClick
};

typedef void (__stdcall *CMouseButtonCallback)(int x, int y, int Button, EMouseAction Action);

namespace App
{
// Application handle for API interaction
typedef void* const CIDEAppHandle;

#define DeclareCIDEApp(Handle) App::CCIDEApp* const CIDEApp = App::CIDEAppFromHandle(Handle)

class CCIDEApp//: public App::CApplication
{
	//__DeclareSingleton(CCIDEApp);

private:

	CAppFSM							FSM;
	HWND							ParentHwnd;

	Ptr<Story::CQuestSystem>		QuestSystem;
	Ptr<Story::CDlgSystem>			DlgSystem;
	Ptr<Items::CItemManager>		ItemManager;

	// We may subscribe to this callbacks before CDataServer is initialized. So it's temporary stored here.
	Data::CDataPathCallback			DataPathCB;
	Data::CReleaseMemoryCallback	ReleaseMemoryCB;

public:

	Game::PEntity					EditorCamera;
	Game::PEntity					CurrentEntity;
	Game::PEntity					CurrentTfmEntity;
	CMouseButtonCallback			MouseCB;

	Data::PParams					AttrDescs;

	bool							TransformMode;
	bool							LimitToGround;
	bool							SnapToGround;
	nString							OldInputPropClass;

	CCIDEApp();
	~CCIDEApp();

	nString	GetAppName() const { return "DeusExMachina API Library"; }
	nString	GetAppVersion() const;
	nString	GetVendorName() const { return "STILL NO TEAM NAME"; }

	void	SetParentWindow(HWND ParentWnd) { ParentHwnd = ParentWnd; }

	bool	Open();
	void	RegisterAttributes();
	void	SetupDisplayMode();
	bool	AdvanceFrame();
	void	Close();
	void	SetDataPathCB(Data::CDataPathCallback Cb, Data::CReleaseMemoryCallback ReleaseCb);
};

typedef CCIDEApp* PCIDEApp;

//!!! Do not use AppInst in any cases except creating and disposing. It is for consistence check only. !!!
extern PCIDEApp AppInst;

inline nString CCIDEApp::GetAppVersion() const
{
	nString Ver;
#ifdef _DEBUG
	Ver.Format("%d.%dd Step %d", DEM_API_VER_MAJOR, DEM_API_VER_MINOR, DEM_API_VER_STEP);
#else
	Ver.Format("%d.%d Step %d", DEM_API_VER_MAJOR, DEM_API_VER_MINOR, DEM_API_VER_STEP);
#endif
	return Ver;
}
//---------------------------------------------------------------------

inline CCIDEApp* const CIDEAppFromHandle(CIDEAppHandle Handle)
{
	n_assert(Handle);
	CCIDEApp* pApp = static_cast<CCIDEApp* const>(Handle);
	// Only one handle may exist simultaneously. All other handles are invalid.
	n_assert(pApp == AppInst);
	return pApp;
}
//---------------------------------------------------------------------

}

#endif