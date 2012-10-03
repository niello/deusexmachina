#include <App/AppFSM.h>
#include <Story/Dlg/DlgSystem.h>
#include <Story/Quests/QuestSystem.h>
#include <Items/ItemManager.h>
#include <Game/Entity.h>
#include <Data/Singleton.h>
#include <StdAPI.h>

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

#define CIDEApp App::CCIDEApp::Instance()

class CCIDEApp//: public App::CApplication
{
	__DeclareSingleton(CCIDEApp);

private:

	CAppFSM						FSM;
	HWND						ParentHwnd;

	Ptr<Story::CQuestSystem>	QuestSystem;
	Ptr<Story::CDlgSystem>		DlgSystem;
	Ptr<Items::CItemManager>	ItemManager;

public:

	Game::PEntity				EditorCamera;
	Game::PEntity				CurrentEntity;
	Game::PEntity				CurrentTfmEntity;
	CMouseButtonCallback		MouseCB;

	Data::PParams				AttrDescs;

	bool						TransformMode;
	bool						LimitToGround;
	bool						SnapToGround;
	nString						OldInputPropClass;

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
};

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

}