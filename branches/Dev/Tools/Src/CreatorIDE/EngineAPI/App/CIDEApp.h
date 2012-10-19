#include <App/AppFSM.h>
#include <Story/Dlg/DlgSystem.h>
#include <Story/Quests/QuestSystem.h>
#include <Items/ItemManager.h>
#include <Game/Entity.h>
#include <Data/Singleton.h>
#include <StdAPI.h>
#include <AI/Navigation/NavMesh.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetClassName
#undef CopyFile
#undef CreateDirectory

namespace App
{
class CCSharpUIConnector;

struct CLevelInfo
{
	nString						ID;
	nArray<CConvexVolume>		ConvexVolumes;
	nArray<COffmeshConnection>	OffmeshConnections;
	bool						ConvexChanged;
	bool						OffmeshChanged;
	bool						NavGeometryChanged;
};

#define CIDEApp App::CCIDEApp::Instance()

class CCIDEApp
{
	__DeclareSingleton(CCIDEApp);

private:

	CAppFSM						FSM;
	HWND						ParentHwnd;
	CCSharpUIConnector*		pUIConnector;
	CNavMeshBuilder*			pNavMeshBuilder;

	Ptr<Story::CQuestSystem>	QuestSystem;
	Ptr<Story::CDlgSystem>		DlgSystem;
	Ptr<Items::CItemManager>	ItemManager;

public:

	Data::PParams				AttrDescs;		// DB attributes additional info for UI and editor

	DB::PDataset				Levels;
	CLevelInfo					CurrLevel;

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

	bool	LoadLevel(const nString& ID);
	void	UnloadLevel(bool SaveChanges);
	int		GetLevelCount() const;
	bool	BuildNavMesh(const char* pRsrcName, float AgentRadius, float AgentHeight, float MaxClimb);
	void	InvalidateNavGeometry();

	nString	GetStringInput(const char* pInitial);

	CCSharpUIConnector* GetUIEventHandler() { return pUIConnector; }
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