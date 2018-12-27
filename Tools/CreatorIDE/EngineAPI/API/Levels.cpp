#include <DB/StdAttrs.h>
#include <Loading/LoaderServer.h>
#include <App/CIDEApp.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/FocusManager.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <AI/AIServer.h>
#include <DetourNavMeshQuery.h>

namespace Attr
{
	DeclareAttr(Name);
	DeclareAttr(Center);
	DeclareAttr(Extents);
	DeclareAttr(NavMesh);
}

API int Levels_GetCount()
{
	return CIDEApp->GetLevelCount();
}
//---------------------------------------------------------------------

API void Levels_GetIDName(int Idx, char* OutID, char* OutName)
{
	CIDEApp->Levels->SetRowIndex(Idx);
	LPCSTR IDStr = CIDEApp->Levels->Get<CStrID>(Attr::GUID).CStr();
	if (IDStr) strcpy(OutID, IDStr);
	LPCSTR NameStr = CIDEApp->Levels->Get<nString>(Attr::Name).Get();
	if (NameStr) strcpy(OutName, NameStr);
}
//---------------------------------------------------------------------

API void Levels_LoadLevel(const char* LevelID)
{
	CIDEApp->LoadLevel(LevelID);
}
//---------------------------------------------------------------------

API bool Levels_CreateNew(const char* ID, const char* Name, float Center[3],
						  float Extents[3], const char* NavMesh)
{
	DB::PDataset DS = CIDEApp->Levels;

	// Search for a duplicate IDs
	for (int i = 0; i < DS->GetRowCount(); ++i)
	{
		DS->SetRowIndex(i);
		CStrID LvlID = CIDEApp->Levels->Get<CStrID>(Attr::GUID);
		if (LvlID == ID) FAIL;
	}

	DS->AddRow();
	DS->Set<CStrID>(Attr::GUID, CStrID(ID));
	DS->Set<nString>(Attr::Name, Name);
	DS->Set<vector3>(Attr::Center, vector3(Center[0], Center[1], Center[2]));
	DS->Set<vector3>(Attr::Extents, vector3(Extents[0], Extents[1], Extents[2]));
	DS->Set<nString>(Attr::NavMesh, NavMesh);
	DS->CommitChanges();

	OK;
}
//---------------------------------------------------------------------

API void Levels_RestoreDB(const char* LevelID)
{
	CIDEApp->UnloadLevel(false);
	LoaderSrv->NewGame(LevelID);
	EntityMgr->AttachEntity(CIDEApp->EditorCamera);
	FocusMgr->SetFocusEntity(CIDEApp->EditorCamera);
}
//---------------------------------------------------------------------

API void Levels_SaveDB(const char* LevelID)
{
	//!!!Equal to LoaderSrv->SaveGame!
	LoaderSrv->CommitChangesToDB();
	n_assert(DataSrv->CopyFile(LoaderSrv->GetDatabasePath(), nString("export:db/") + LoaderSrv->GetGameDBName() + ".db3"));
}
//---------------------------------------------------------------------

API bool Levels_BuildNavMesh(const char* RsrcName, float AgentRadius, float AgentHeight, float MaxClimb)
{
	return CIDEApp->BuildNavMesh(RsrcName, AgentRadius, AgentHeight, MaxClimb);
}
//---------------------------------------------------------------------

API int Levels_GetNavPolyUnderMouse()
{
	dtNavMeshQuery* pNavQuery = AISrv->GetLevel()->GetSyncNavQuery(0.3f);
	const dtQueryFilter* pNavFilter = AISrv->GetDefaultNavQueryFilter();
	if (!pNavQuery || !pNavFilter) return 0;

	dtPolyRef Ref;
	const float Extents[3] = { 0.f, 3.f, 0.f };
	pNavQuery->findNearestPoly(EnvQueryMgr->GetMousePos3D().v, Extents, pNavFilter, &Ref, NULL);
	return Ref;
}
//---------------------------------------------------------------------
