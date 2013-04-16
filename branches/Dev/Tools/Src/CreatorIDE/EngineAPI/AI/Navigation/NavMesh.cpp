#include "NavMesh.h"

#include <Physics/Prop/PropAbstractPhysics.h>
#include <Game/Entity.h>
//#include <RecastDump.h>

using namespace Properties;

// Returns true if 'c' is left of line 'a'-'b'.
inline bool left(const float* a, const float* b, const float* c)
{ 
	const float u1 = b[0] - a[0];
	const float v1 = b[2] - a[2];
	const float u2 = c[0] - a[0];
	const float v2 = c[2] - a[2];
	return u1 * v2 - v1 * u2 < 0;
}
//---------------------------------------------------------------------

// Returns true if 'a' is more lower-left than 'b'.
inline bool cmppt(const float* a, const float* b)
{
	if (a[0] < b[0]) return true;
	if (a[0] > b[0]) return false;
	if (a[2] < b[2]) return true;
	if (a[2] > b[2]) return false;
	return false;
}
//---------------------------------------------------------------------

int ConvexHull(const vector3* pts, int npts, int* out)
{
	// Find lower-leftmost point.
	int hull = 0;
	for (int i = 1; i < npts; ++i)
		if (cmppt(pts[i].v, pts[hull].v))
			hull = i;
	// Gift wrap hull.
	int endpt = 0;
	int i = 0;
	do
	{
		out[i++] = hull;
		endpt = 0;
		for (int j = 1; j < npts; ++j)
			if (hull == endpt || left(pts[hull].v, pts[endpt].v, pts[j].v))
				endpt = j;
		hull = endpt;
	}
	while (endpt != out[0]);
	
	return i;
}
//---------------------------------------------------------------------

CNavMeshBuilder::CNavMeshBuilder():
	pHF(NULL),
	pCompactHF(NULL),
	pMesh(NULL),
	pMeshDetail(NULL),
	OffMeshCount(0),
	Radius(0.f),
	Height(0.f)
{
	memset(&Params, 0, sizeof(Params));
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::Init(const rcConfig& Config, float MaxClimb)
{
	Cfg = Config;

	Climb = MaxClimb;
	Cfg.walkableClimb = (int)floorf(Climb / Cfg.ch);

	rcCalcGridSize(Cfg.bmin, Cfg.bmax, Cfg.cs, &Cfg.width, &Cfg.height);

	Ctx.resetTimers();

	Ctx.startTimer(RC_TIMER_TOTAL);
	
	Ctx.log(RC_LOG_PROGRESS, "Building navigation:");
	Ctx.log(RC_LOG_PROGRESS, " - %d x %d cells", Cfg.width, Cfg.height);

	if (pHF) rcFreeHeightField(pHF);
	pHF = rcAllocHeightfield();
	if (!pHF)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'rcHeightfield'.");
		FAIL;
	}

	if (!rcCreateHeightfield(&Ctx, *pHF, Cfg.width, Cfg.height, Cfg.bmin, Cfg.bmax, Cfg.cs, Cfg.ch))
	{
		rcFreeHeightField(pHF);
		pHF = NULL;
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

/*
bool CNavMeshBuilder::AddGeometry(nSceneNode* pNode, const matrix44* pTfm, uchar Area)
{
	if (pNode->IsA(pCLODClass))
	{
		nChunkLodTree* pTree = ((nTerrainNode*)pNode)->GetChunkLodTree();
		if (!AddGeometryNCT2(pTree->GetRootChunkLodNode(), pTree, pTfm, Area)) FAIL;
	}
	else if (pNode->IsA(pSkinShapeClass) || pNode->IsA(pSkyClass))
	{
		//!!!NOTE - skinned characters must not be detected as static geometry!
		// Force skip skinned. Skinned must be processed as collision geometry, not as gfx.
		// Force skip sky. Sky must not be processed.
	}
	else if (pNode->IsA(pShapeClass))
	{
		if (!AddGeometry(((nShapeNode*)pNode)->GetMeshObject(), ((nShapeNode*)pNode)->groupIndex, false, pTfm, Area)) FAIL;
	}
	else if (pNode->IsA(pTfmClass))
	{
		matrix44 Tmp;
		const matrix44& NodeTfm = ((nTransformNode*)pNode)->GetTransform();
		if (NodeTfm != matrix44::identity)
		{
			if (pTfm)
			{
				Tmp = (*pTfm) * NodeTfm;
				pTfm = &Tmp;
			}
			else pTfm = &NodeTfm;
		}
		
		for (nSceneNode* pCurrNode = (nSceneNode*)pNode->GetHead(); pCurrNode; pCurrNode = (nSceneNode*)pCurrNode->GetSucc())
			if (!AddGeometry(pCurrNode, pTfm, Area)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------
*/

bool CNavMeshBuilder::AddGeometry(Game::CEntity& Entity, uchar Area)
{
	const matrix44& Tfm = Entity.Get<matrix44>(Attr::Transform);
	const matrix44* pTfm = (Tfm == matrix44::identity) ? NULL : &Tfm; // Geometry transform optimization

	// Logically must get collision geometry

	// For meshes get pTris from the mesh itself
	// For boxes will get 12 pTris per box
	// For cylinders, capsules, spheres etc get gfx pTris for now

	CPropAbstractPhysics* pPropPhys = Entity.FindProperty<CPropAbstractPhysics>();
	if (pPropPhys)
	{
		// return, if processed
	}

	//GFX //!!!parse scene nodes!
	/*
	CPropGraphics* pPropGfx = Entity.FindProperty<CPropGraphics>();
	if (pPropGfx)
	{
		const CGfxShapeArray& GfxEnts = pPropGfx->GetGfxEntities();
		for (int i = 0; i < GfxEnts.Size(); ++i)
			AddGeometry(GfxEnts[i]->GetResource().GetNode(), pTfm, Area);
	}
	*/

	OK;
}
//---------------------------------------------------------------------

/*
bool CNavMeshBuilder::AddGeometry(nMesh2* pMesh, int GroupIdx, bool IsStrip, const matrix44* pTfm, uchar Area)
{
	nMeshGroup Group;
	if (GroupIdx == -1)
	{
		Group.FirstVertex = 0;
		Group.FirstIndex = 0;
		Group.NumVertices = pMesh->GetNumVertices();
		Group.NumIndices = pMesh->GetNumIndices();
	}
	else if (GroupIdx >= pMesh->GetNumGroups()) FAIL;
	else Group = pMesh->Group(GroupIdx);

	int OldUsage = pMesh->GetUsage();
	pMesh->SetUsage(nMesh2::ReadOnly);

	// Copy vertices, position component only, with optional transformation
	float* pVertices = (float*)n_malloc(3 * Group.NumVertices * sizeof(float));
	float* pCurrVtx = pVertices;
	float* pVBuf = pMesh->LockVertices() + Group.FirstVertex * pMesh->GetVertexWidth();
	float* pVEnd = pVBuf + Group.NumVertices * pMesh->GetVertexWidth();
	pVBuf += pMesh->GetVertexComponentOffset(nMesh2::Coord);
	for (; pVBuf < pVEnd; pVBuf += pMesh->GetVertexWidth())
	{
		if (pTfm)
		{
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][0] + pVBuf[1] * pTfm->m[1][0] + pVBuf[2] * pTfm->m[2][0] + pTfm->m[3][0];
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][1] + pVBuf[1] * pTfm->m[1][1] + pVBuf[2] * pTfm->m[2][1] + pTfm->m[3][1];
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][2] + pVBuf[1] * pTfm->m[1][2] + pVBuf[2] * pTfm->m[2][2] + pTfm->m[3][2];
		}
		else
		{
			*pCurrVtx++ = pVBuf[0];
			*pCurrVtx++ = pVBuf[1];
			*pCurrVtx++ = pVBuf[2];
		}
	}
	pMesh->UnlockVertices();

	// Copy indices with conversion to TriList
	int* pIndices = NULL;
	int TriCount = 0;
	ushort *pIBuf = pMesh->LockIndices() + Group.FirstIndex;
	if (IsStrip)
	{
		//!!!DUPLICATE CODE!
		TriCount = Group.NumIndices - 2;
		pIndices = (int*)n_malloc(3 * TriCount * sizeof(int));
		int* pCurrIdx = pIndices;
		bool Odd = true;
		for (int i = 0; i < TriCount; ++i, Odd = !Odd)
		{
			*pCurrIdx++ = pIBuf[i];
			if (Odd)
			{
				*pCurrIdx++ = pIBuf[i + 1];
				*pCurrIdx++ = pIBuf[i + 2];
			}
			else
			{
				*pCurrIdx++ = pIBuf[i + 2];
				*pCurrIdx++ = pIBuf[i + 1];
			}
		}
	}
	else
	{
		TriCount = Group.NumIndices / 3;
		pIndices = (int*)n_malloc(Group.NumIndices * sizeof(int));
		int* pCurrIdx = pIndices;
		for (int i = 0; i < Group.NumIndices; ++i)
			*pCurrIdx++ = (int)*pIBuf++;

		//// Invert triangles
		//for (int i = 0; i < TriCount; ++i)
		//{
		//	*pCurrIdx++ = (int)pIBuf[i * 3];
		//	*pCurrIdx++ = (int)pIBuf[i * 3 + 2];
		//	*pCurrIdx++ = (int)pIBuf[i * 3 + 1];
		//}
	}
	pMesh->UnlockIndices();

	bool Result = AddGeometry(pVertices, Group.NumVertices, pIndices, TriCount, Area);

	n_free(pVertices);
	n_free(pIndices);

	pMesh->SetUsage(OldUsage);

	return Result;
}
//---------------------------------------------------------------------
*/

bool CNavMeshBuilder::AddGeometry(const float* pVerts, int VertexCount, const int* pTris, int TriCount, uchar Area)
{
	n_assert_dbg(pVerts && pTris && VertexCount > 0 && TriCount > 0);

	// Can allocate once and grow as triangle count exceeds allocated
	// Or can allocate for the max triangle count once
	uchar* pAreas = n_new_array(uchar, TriCount);
	if (!pAreas)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pAreas' (%d).", TriCount);
		FAIL;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(pAreas, Area, TriCount * sizeof(uchar));
	if (Area == RC_NULL_AREA)
		rcMarkWalkableTriangles(&Ctx, Cfg.walkableSlopeAngle, pVerts, VertexCount, pTris, TriCount, pAreas);
	else
		rcClearUnwalkableTriangles(&Ctx, Cfg.walkableSlopeAngle, pVerts, VertexCount, pTris, TriCount, pAreas);
	rcRasterizeTriangles(&Ctx, pVerts, VertexCount, pTris, pAreas, TriCount, *pHF, Cfg.walkableClimb);

	n_delete_array(pAreas);

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::PrepareGeometry(float AgentRadius, float AgentHeight)
{
	//	Ctx.log(RC_LOG_PROGRESS, " - %.1fK pVerts, %.1fK pTris", VertexCount/1000.0f, TriCount/1000.0f);

	Radius = AgentRadius;
	Height = AgentHeight;
	Cfg.walkableRadius = (int)ceilf(Radius / Cfg.cs);
	Cfg.walkableHeight = (int)ceilf(Height / Cfg.ch);

	rcFilterLowHangingWalkableObstacles(&Ctx, Cfg.walkableClimb, *pHF);
	rcFilterLedgeSpans(&Ctx, Cfg.walkableHeight, Cfg.walkableClimb, *pHF);
	rcFilterWalkableLowHeightSpans(&Ctx, Cfg.walkableHeight, *pHF);

	if (pCompactHF) rcFreeCompactHeightfield(pCompactHF);
	pCompactHF = rcAllocCompactHeightfield();
	if (!pCompactHF)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		FAIL;
	}

	if (!rcBuildCompactHeightfield(&Ctx, Cfg.walkableHeight, Cfg.walkableClimb, *pHF, *pCompactHF))
	{
		rcFreeCompactHeightfield(pCompactHF);
		pCompactHF = NULL;
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		FAIL;
	}

	if (!rcErodeWalkableArea(&Ctx, Cfg.walkableRadius, *pCompactHF))
	{
		rcFreeCompactHeightfield(pCompactHF);
		pCompactHF = NULL;
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CNavMeshBuilder::ResetAllArea(uchar NewAreaValue)
{
	rcMarkBoxArea(&Ctx, Cfg.bmin, Cfg.bmax, NewAreaValue, *pCompactHF);
}
//---------------------------------------------------------------------

void CNavMeshBuilder::ApplyConvexVolumeArea(CConvexVolume& Volume)
{
	n_assert_dbg(pCompactHF);
	rcMarkConvexPolyArea(&Ctx, Volume.Vertices->v, Volume.VertexCount, Volume.MinY, Volume.MaxY, Volume.Area, *pCompactHF);
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::AddOffmeshConnection(COffmeshConnection& Connection)
{
	if (OffMeshCount >= MAX_OFFMESH_CONNECTIONS) FAIL;

	float* v = &OffMeshVerts[OffMeshCount * 3 * 2];
	rcVcopy(&v[0], Connection.From.v);
	rcVcopy(&v[3], Connection.To.v);
	OffMeshRadius[OffMeshCount] = Connection.Radius;
	OffMeshDir[OffMeshCount] = Connection.Bidirectional ? 1 : 0;
	OffMeshArea[OffMeshCount] = Connection.Area;
	OffMeshFlags[OffMeshCount] = Connection.Flags;
	OffMeshID[OffMeshCount] = 1000 + OffMeshCount;

	OffMeshCount++;
	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::BuildNavMesh(bool MonotonePartitioning)
{
	if (MonotonePartitioning)
	{
		if (!rcBuildRegionsMonotone(&Ctx, *pCompactHF, 0, Cfg.minRegionArea, Cfg.mergeRegionArea))
		{
			Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
			FAIL;
		}

		n_printf("NavMesh rcBuildRegionsMonotone done\n");
	}
	else
	{
		if (!rcBuildDistanceField(&Ctx, *pCompactHF))
		{
			Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			FAIL;
		}

		n_printf("NavMesh rcBuildDistanceField done\n");

		if (!rcBuildRegions(&Ctx, *pCompactHF, 0, Cfg.minRegionArea, Cfg.mergeRegionArea))
		{
			Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
			FAIL;
		}

		n_printf("NavMesh rcBuildRegions done\n");
	}

	rcContourSet* pContourSet = rcAllocContourSet();
	if (!pContourSet)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		FAIL;
	}

	if (!rcBuildContours(&Ctx, *pCompactHF, Cfg.maxSimplificationError, Cfg.maxEdgeLen, *pContourSet))
	{
		rcFreeContourSet(pContourSet);
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		FAIL;
	}

	n_printf("NavMesh rcBuildContours done\n");

	if (pMesh) rcFreePolyMesh(pMesh);
	pMesh = rcAllocPolyMesh();
	if (!pMesh)
	{
		rcFreeContourSet(pContourSet);
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		FAIL;
	}

	if (!rcBuildPolyMesh(&Ctx, *pContourSet, Cfg.maxVertsPerPoly, *pMesh))
	{
		rcFreeContourSet(pContourSet);
		rcFreePolyMesh(pMesh);
		pMesh = NULL;
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		FAIL;
	}

	rcFreeContourSet(pContourSet);

	n_printf("NavMesh rcBuildPolyMesh done\n");

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::BuildDetailMesh()
{
	if (pMeshDetail) rcFreePolyMeshDetail(pMeshDetail);
	pMeshDetail = rcAllocPolyMeshDetail();
	if (!pMeshDetail)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		FAIL;
	}

	if (!rcBuildPolyMeshDetail(&Ctx, *pMesh, *pCompactHF, Cfg.detailSampleDist, Cfg.detailSampleMaxError, *pMeshDetail))
	{
		rcFreePolyMeshDetail(pMeshDetail);
		pMeshDetail = NULL;
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		FAIL;
	}

	n_printf("NavMesh rcBuildPolyMeshDetail done\n");

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::GetNavMeshData(uchar*& pOutData, int& OutSize)
{
	if (!pMesh) FAIL;

	// Update poly flags from areas.
	for (int i = 0; i < pMesh->npolys; ++i)
	{
		//if (pMesh->areas[i] == RC_WALKABLE_AREA) pMesh->flags[i] = NAV_FLAG_NORMAL;
		//else if (pMesh->areas[i] == NAV_AREA_NAMED) pMesh->flags[i] = NAV_FLAG_NORMAL;
		pMesh->flags[i] = (pMesh->areas[i] == RC_NULL_AREA) ? 0 : NAV_FLAG_NORMAL;
	}

	//!!!Update ofmesh poly flags if not set!

	Params.walkableHeight = Height;
	Params.walkableRadius = Radius;
	Params.walkableClimb = Climb;
	Params.cs = Cfg.cs;
	Params.ch = Cfg.ch;
	Params.buildBvTree = true;
	rcVcopy(Params.bmin, pMesh->bmin);
	rcVcopy(Params.bmax, pMesh->bmax);

	Params.verts = pMesh->verts;
	Params.vertCount = pMesh->nverts;
	Params.polys = pMesh->polys;
	Params.polyAreas = pMesh->areas;
	Params.polyFlags = pMesh->flags;
	Params.polyCount = pMesh->npolys;
	Params.nvp = pMesh->nvp;

	if (pMeshDetail)
	{
		Params.detailMeshes = pMeshDetail->meshes;
		Params.detailVerts = pMeshDetail->verts;
		Params.detailVertsCount = pMeshDetail->nverts;
		Params.detailTris = pMeshDetail->tris;
		Params.detailTriCount = pMeshDetail->ntris;
	}

	Params.offMeshConVerts = OffMeshVerts;
	Params.offMeshConRad = OffMeshRadius;
	Params.offMeshConDir = OffMeshDir;
	Params.offMeshConAreas = OffMeshArea;
	Params.offMeshConFlags = OffMeshFlags;
	Params.offMeshConUserID = OffMeshID;
	Params.offMeshConCount = OffMeshCount;

	//???need to free somewhere?!
	if (!dtCreateNavMeshData(&Params, &pOutData, &OutSize))
	{
		Ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
		FAIL;
	}

	n_printf("NavMesh dtCreateNavMeshData done\n");

	//duLogBuildTimes(*Ctx, Ctx.getAccumulatedTime(RC_TIMER_TOTAL));
	Ctx.log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", pMesh->nverts, pMesh->npolys);

	OK;
}
//---------------------------------------------------------------------

void CNavMeshBuilder::Cleanup()
{
	if (pMeshDetail)
	{
		rcFreePolyMeshDetail(pMeshDetail);
		pMeshDetail = NULL;
	}

	if (pMesh)
	{
		rcFreePolyMesh(pMesh);
		pMesh = NULL;
	}

	if (pCompactHF)
	{
		rcFreeCompactHeightfield(pCompactHF);
		pCompactHF = NULL;
	}

	if (pHF)
	{
		rcFreeHeightField(pHF);
		pHF = NULL;
	}

	ClearOffmeshConnections();

	memset(&Params, 0, sizeof(Params));
}
//---------------------------------------------------------------------
