#pragma once
#ifndef __CIDE_APP_H__
#define __CIDE_APP_H__

#include <App/AppFSM.h>
#include <Dlg/DialogueManager.h>
#include <Quests/QuestManager.h>
#include <Items/ItemManager.h>
#include <Game/Entity.h>
#include <Core/Singleton.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>
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
	__DeclareSingleton(CCIDEApp);

private:

	CAppFSM							FSM;
	HWND							ParentHwnd;

	Ptr<Story::CQuestManager>		QuestSystem;
	Ptr<Story::CDialogueManager>	DlgSystem;
	Ptr<Items::CItemManager>		ItemManager;

	// We may subscribe to this callbacks before CDataServer is initialized. So it's temporary stored here.
	IO::CDataPathCallback			DataPathCB;
	IO::CReleaseMemoryCallback		ReleaseMemoryCB;

public:

	Game::PEntity					EditorCamera;
	Game::PEntity					CurrentEntity;	// Entity on which entity operations are performed (attr read/write etc)
	Game::PEntity					SelectedEntity;	// Entity selected in the editor window
	Game::PEntity					CurrentTfmEntity;
	CMouseButtonCallback			MouseCB;

	Data::PParams					AttrDescs;

	bool							TransformMode;
	bool							LimitToGround;
	bool							SnapToGround;

	// Ground constraints for the selected entity transformation
	bool						DenyEntityAboveGround;
	bool						DenyEntityBelowGround;

	CCIDEApp();
	~CCIDEApp();

	CString	GetAppName() const { return "DeusExMachina API Library"; }
	CString	GetAppVersion() const;
	CString	GetVendorName() const { return "STILL NO TEAM NAME"; }

	void	SetParentWindow(HWND ParentWnd) { ParentHwnd = ParentWnd; }

	void	ApplyGroundConstraints(const Game::CEntity& Entity, vector3& Position);

	bool	Open();
	void	RegisterAttributes();
	void	SetupDisplayMode();
	bool	AdvanceFrame();
	void	Close();
	void	SetDataPathCB(IO::CDataPathCallback Cb, IO::CReleaseMemoryCallback ReleaseCb);
};

typedef CCIDEApp* PCIDEApp;

//!!! Do not use AppInst in any cases except creating and disposing. It is for consistence check only. !!!
extern PCIDEApp AppInst;

inline CString CCIDEApp::GetAppVersion() const
{
	CString Ver;
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