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

namespace App
{
class CCSharpUIEventHandler;

#define CIDEApp App::CCIDEApp::Instance()

class CCIDEApp
{
	__DeclareSingleton(CCIDEApp);

private:

	CAppFSM						FSM;
	HWND						ParentHwnd;

	Ptr<Story::CQuestSystem>	QuestSystem;
	Ptr<Story::CDlgSystem>		DlgSystem;
	Ptr<Items::CItemManager>	ItemManager;

	CCSharpUIEventHandler*		pUIEventHandler;

public:

	Data::PParams				AttrDescs;		// DB attributes additional info for UI and editor

	Game::PEntity				EditorCamera;
	Game::PEntity				CurrentEntity;	// Entity on which entity operations are performed (attr read/write etc)
	Game::PEntity				SelectedEntity;	// Entity selected in the editor window

	// Ground constraints for the selected entity transformation
	bool						DenyEntityAboveGround;
	bool						DenyEntityBelowGround;

	CCIDEApp();
	~CCIDEApp();

	nString	GetAppName() const { return "DeusExMachina API Library"; }
	nString	GetAppVersion() const;
	nString	GetVendorName() const { return "STILL NO TEAM NAME"; }

	void	SetParentWindow(HWND ParentWnd) { ParentHwnd = ParentWnd; }

	bool	Open();
	bool	AdvanceFrame() { return FSM.Advance(); }
	void	Close();

	bool	SetEditorTool(LPCSTR Name); //!!!RTTI!
	bool	SelectEntity(Game::PEntity Entity);
	void	ClearSelectedEntities();

	void	ApplyGroundConstraints(const Game::CEntity& Entity, vector3& Position);

	CCSharpUIEventHandler* GetUIEventHandler() { return pUIEventHandler; }
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