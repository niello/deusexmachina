#include <StdAPI.h>
#include <DB/DBServer.h>
#include <DB/Database.h>
#include <DB/StdAttrs.h>
#include <Data/Streams/FileStream.h>
#include <Loading/LoaderServer.h>
#include <App/CIDEApp.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Game/Mgr/FocusManager.h>
#include <AI/Navigation/NavMesh.h>
#include <DetourNavMesh.h> // For max verts per poly const

namespace Attr
{
	DeclareAttr(Name);
	DeclareAttr(Center);
	DeclareAttr(Extents);
	DeclareAttr(NavMesh);
}

API void Transform_SetCurrentEntity(const char* UID);

//!!!too slow and ugly!
API int Levels_GetCount()
{
	int TblIdx = LoaderSrv->GetGameDB()->FindTableIndex("Levels");
	
	if (TblIdx == INVALID_INDEX) return 0;
	
	DB::PDataset DS = LoaderSrv->GetGameDB()->GetTable(TblIdx)->CreateDataset();
	DS->PerformQuery();
	return DS->GetRowCount();
}
//---------------------------------------------------------------------

//!!!too slow and ugly!
API void Levels_GetIDName(int Idx, char* OutID, char* OutName)
{
	DB::PDataset DS = LoaderSrv->GetGameDB()->GetTable("Levels")->CreateDataset();
	DS->AddColumn(Attr::GUID);
	DS->AddColumn(Attr::Name);
	DS->PerformQuery();
	DS->SetRowIndex(Idx);
	LPCSTR IDStr = DS->Get<CStrID>(Attr::GUID).CStr(),
		   NameStr = DS->Get<nString>(Attr::Name).Get();
	if(IDStr) strcpy(OutID, IDStr);
	if(NameStr) strcpy(OutName, NameStr);
}
//---------------------------------------------------------------------

API void Levels_LoadLevel(const char* LevelID)
{
	LoaderSrv->CommitChangesToDB();
	Transform_SetCurrentEntity(NULL);
	LoaderSrv->UnloadLevel();
	LoaderSrv->LoadLevel(LevelID);
	EntityMgr->AttachEntity(CIDEApp->EditorCamera);
	FocusMgr->SetFocusEntity(CIDEApp->EditorCamera);
}
//---------------------------------------------------------------------

API bool Levels_CreateNew(const char* ID, const char* Name, float Center[3],
						  float Extents[3], const char* NavMesh)
{
	//!!!check duplicate level!
	
	DB::PTable Tbl;
	int TblIdx = LoaderSrv->GetGameDB()->FindTableIndex("Levels");
	if (TblIdx == INVALID_INDEX)
	{
		Tbl = DB::CTable::Create();
		Tbl->SetName("Levels");
		Tbl->AddColumn(DB::CColumn(Attr::GUID, DB::CColumn::Primary));
		Tbl->AddColumn(Attr::Name);
		Tbl->AddColumn(Attr::Center);
		Tbl->AddColumn(Attr::Extents);
		Tbl->AddColumn(Attr::NavMesh);
		LoaderSrv->GetGameDB()->AddTable(Tbl);
	}
	else Tbl = LoaderSrv->GetGameDB()->GetTable(TblIdx);

	DB::PDataset DS = Tbl->CreateDataset();
	DS->AddColumnsFromTable();
	DS->AddRow();
	DS->Set<CStrID>(Attr::GUID, CStrID(ID));
	DS->Set<nString>(Attr::Name, Name);
	DS->Set<vector3>(Attr::Center, vector3(Center[0], Center[1], Center[2]));
	DS->Set<vector3>(Attr::Extents, vector3(Extents[0], Extents[1], Extents[2]));
	DS->Set<nString>(Attr::NavMesh, NavMesh);
	DS->CommitChanges();
	//???LoaderSrv->CommitChangesToGameDB();
	OK;
}
//---------------------------------------------------------------------

API void Levels_RestoreDB(const char* LevelID)
{
	Transform_SetCurrentEntity(NULL);
	LoaderSrv->UnloadLevel();
	LoaderSrv->NewGame(LevelID);
	EntityMgr->AttachEntity(CIDEApp->EditorCamera);
	FocusMgr->SetFocusEntity(CIDEApp->EditorCamera);
}
//---------------------------------------------------------------------

API void Levels_SaveDB(const char* LevelID)
{
	LoaderSrv->CommitChangesToDB();
	nString DBPath = nString("export:db/") + LoaderSrv->GetGameDBName() + ".db3";
	n_assert(DataSrv->CopyFile(LoaderSrv->GetDatabasePath(), DBPath));
}
//---------------------------------------------------------------------

API bool Levels_BuildNavMesh(const char* RsrcName, float AgentRadius, float AgentHeight, float MaxClimb)
{
	//!!!Check if no level is loaded or no geom to process!
	//if (!m_geom || !m_geom->getMesh())
	//{
	//	Ctx.log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
	//	FAIL;
	//}

	//m_cellSize = 0.3f;
	//m_cellHeight = 0.2f;
	//m_agentHeight = 2.0f;
	//m_agentRadius = 0.6f;
	//m_agentMaxClimb = 0.9f;
	//m_agentMaxSlope = 45.0f;
	//m_regionMinSize = 8;
	//m_regionMergeSize = 20;
	//m_monotonePartitioning = false;
	//m_edgeMaxLen = 12.0f;
	//m_edgeMaxError = 1.3f;
	//m_vertsPerPoly = 6.0f;
	//m_detailSampleDist = 6.0f;
	//m_detailSampleMaxError = 1.0f;

	rcConfig Cfg;
	memset(&Cfg, 0, sizeof(Cfg));

	//!!!need to calc from geometry selected!
	const bbox3& Box = LoaderSrv->GetCurrentLevelBox();
	rcVcopy(Cfg.bmin, Box.vmin.v);
	rcVcopy(Cfg.bmax, Box.vmax.v);

	float BoxSizeX = Box.vmax.x - Box.vmin.x;
	float BoxSizeZ = Box.vmax.z - Box.vmin.z;
	float BoxSizeMax = n_max(BoxSizeX, BoxSizeZ);
	Cfg.cs = BoxSizeMax * 0.00029f; // nearly one over 3450.f, empirically detected
	Cfg.ch = 0.2f;
	Cfg.walkableSlopeAngle = 45.0f;
	Cfg.maxEdgeLen = (int)(12.0f / Cfg.cs);
	Cfg.maxSimplificationError = 1.3f;

	// empirically detected, one over 133.(3)f
	int Factor = (int)n_floor(BoxSizeMax * 0.0075f + 0.5f); // 0.5f to round 1.5 to 2
	Cfg.minRegionArea = (int)rcSqr(Factor);					// Note: area = size*size

	Cfg.mergeRegionArea = (int)rcSqr(20);				// Note: area = size*size
	Cfg.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
	Cfg.detailSampleDist = 6.0f < 0.9f ? 0.f : Cfg.cs * 6.0f;
	Cfg.detailSampleMaxError = Cfg.ch * 1.0f;

	n_printf("NavMesh building started\n");

	CNavMeshBuilder NMB;
	if (!NMB.Init(Cfg, MaxClimb)) FAIL;

	n_printf("NavMesh building Init done\n");

	const nArray<Game::PEntity>& Ents = EntityMgr->GetEntities();
	for (int i = 0; i < Ents.Size(); ++i)
	{
		Game::CEntity& Ent = *Ents[i];
		if (StaticEnvMgr->IsEntityStatic(Ent))
			NMB.AddGeometry(Ent);
	}

	n_printf("NavMesh geom. added\n");

	//while (0)
	//{
	//	NMB.AddOffmeshConnection();
	//}

	if (!NMB.PrepareGeometry(AgentRadius, AgentHeight)) FAIL;

	n_printf("NavMesh building PrepareGeometry done\n");

	//while (0)
	//{
	//	NMB.ApplyConvexVolumeArea();
	//}

	uchar* pData;
	int Size;
	if (!NMB.Build(pData, Size)) FAIL;

	n_printf("NavMesh building Build done\n");

	nString Path;
	Path.Format("export:Nav/%s.nm", RsrcName);
	n_printf("NavMesh file path: %s\n", DataSrv->ManglePath(Path).Get());
	if (DataSrv->CreateDirectory(Path.ExtractDirName()))
	{
		Data::CFileStream File;
		if (File.Open(Path.Get(), Data::SAM_WRITE))
		{
			File.Write(pData, Size);
			File.Close();
			n_printf("NavMesh saved\n");
		}
	}

	//NMB.Cleanup(); // Will be called in NMB destructor

	//n_assert(nFileServer2::Instance()->CopyFile(LoaderSrv->GetDatabasePath(), DBPath));

	OK;
}
//---------------------------------------------------------------------
