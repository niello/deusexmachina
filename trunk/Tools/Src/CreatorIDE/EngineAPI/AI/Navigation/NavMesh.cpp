#include "NavMesh.h"

#include <Gfx/Prop/PropGraphics.h>
#include <Gfx/ShapeEntity.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Game/Entity.h>
#include <scene/nshapenode.h>
#include <ncterrain2/nterrainnode.h>
#include <ncterrain2/nchunklodnode.h>
//#include <RecastDump.h>
#include <DetourNavMeshBuilder.h>

using namespace Properties;

bool CNavMeshBuilder::Init(const rcConfig& Config, float MaxClimb)
{
	n_assert(!pHF);

	Cfg = Config;

	Climb = MaxClimb;
	Cfg.walkableClimb = (int)floorf(Climb / Cfg.ch);

	rcCalcGridSize(Cfg.bmin, Cfg.bmax, Cfg.cs, &Cfg.width, &Cfg.height);

	Ctx.resetTimers();

	Ctx.startTimer(RC_TIMER_TOTAL);
	
	Ctx.log(RC_LOG_PROGRESS, "Building navigation:");
	Ctx.log(RC_LOG_PROGRESS, " - %d x %d cells", Cfg.width, Cfg.height);

	pHF = rcAllocHeightfield();
	if (!pHF)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'rcHeightfield'.");
		FAIL;
	}

	if (!rcCreateHeightfield(&Ctx, *pHF, Cfg.width, Cfg.height, Cfg.bmin, Cfg.bmax, Cfg.cs, Cfg.ch))
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::AddGeometryNCT2(nChunkLodNode* pNode, nChunkLodTree* pTree, const matrix44* pTfm, uchar Area)
{
	if (pNode->HasChildren())
	{
		for (int i = 0; i < 4; ++i)
			if (!AddGeometryNCT2(pNode->GetChild(i), pTree, pTfm, Area)) FAIL;
	}
	else
	{
		pNode->RequestLoadData(pTree, 1.f);
		nMesh2* pMesh = pNode->GetChunkLodMesh()->GetMesh();
		bool Result = pMesh ? AddGeometry(pMesh, pTfm, Area) : true;
		pNode->RequestUnloadData(pTree);
		return Result;
	}

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::AddGeometry(Game::CEntity& Entity, uchar Area)
{
	const matrix44& Tfm = Entity.Get<matrix44>(Attr::Transform);

	// Logically must get collision geometry

	// For meshes get pTris from the mesh itself
	// For boxes will get 12 pTris per box
	// For cylinders, capsules, spheres etc get gfx pTris for now

	CPropAbstractPhysics* pPropPhys = Entity.FindProperty<CPropAbstractPhysics>();
	if (pPropPhys)
	{
	}

	CPropGraphics* pPropGfx = Entity.FindProperty<CPropGraphics>();
	if (pPropGfx)
	{
		nClass* pCLODClass = nKernelServer::Instance()->FindClass("nterrainnode");
		n_assert(pCLODClass);
		nClass* pShapeClass = nKernelServer::Instance()->FindClass("nshapenode");
		n_assert(pShapeClass);

		const CGfxShapeArray& GfxEnts = pPropGfx->GetGfxEntities();
		for (int i = 0; i < GfxEnts.Size(); ++i)
		{
			nTransformNode* pNode = GfxEnts[i]->GetResource().GetNode();
			nSceneNode* pCurrNode = (nSceneNode*)pNode->GetHead();
			for (; pCurrNode; pCurrNode = (nSceneNode*)pCurrNode->GetSucc())
			{
				if (pCurrNode->IsA(pCLODClass))
				{
					nChunkLodTree* pTree = ((nTerrainNode*)pCurrNode)->GetChunkLodTree();
					if (!AddGeometryNCT2(pTree->GetRootChunkLodNode(), pTree, &Tfm, Area)) FAIL;
				}
				else if (pCurrNode->IsA(pShapeClass))
				{
					//???force skip skinned? skinned must be processed as collision geometry, not as gfx
					if (!AddGeometry(((nShapeNode*)pCurrNode)->GetMeshObject(), &Tfm, Area)) FAIL;
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::AddGeometry(nMesh2* pMesh, const matrix44* pTfm, uchar Area)
{
	int OldUsage = pMesh->GetUsage();
	pMesh->SetUsage(nMesh2::ReadOnly);

	// Copy vertices, position component only, with optional transformation
	float* pVertices = (float*)n_malloc(3 * sizeof(float) * pMesh->GetNumVertices());
	float* pCurrVtx = pVertices;
	float* pVBuf = pMesh->LockVertices();
	float* pVEnd = pVBuf + pMesh->GetNumVertices() * pMesh->GetVertexWidth();
	pVBuf += pMesh->GetVertexComponentOffset(nMesh2::Coord);
	for (; pVBuf < pVEnd; pVBuf += pMesh->GetVertexWidth())
	{
		if (pTfm)
		{
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][0] + pVBuf[1] * pTfm->m[1][0] + pVBuf[2] * pTfm->m[2][0];
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][1] + pVBuf[1] * pTfm->m[1][1] + pVBuf[2] * pTfm->m[2][1];
			*pCurrVtx++ = pVBuf[0] * pTfm->m[0][2] + pVBuf[1] * pTfm->m[1][2] + pVBuf[2] * pTfm->m[2][2];
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
	int TriCount = pMesh->GetNumIndices() - 2;
	int* pIndices = (int*)n_malloc(3 * sizeof(int) * TriCount);
	int* pCurrIdx = pIndices;
	ushort *pIBuf = pMesh->LockIndices();
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
	pMesh->UnlockIndices();

	bool Result = AddGeometry(pVertices, pMesh->GetNumVertices(), pIndices, TriCount, Area);

	n_free(pVertices);
	n_free(pIndices);

	pMesh->SetUsage(OldUsage);

	return Result;
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::AddGeometry(const float* pVerts, int VertexCount, const int* pTris, int TriCount, uchar Area)
{
	// Can allocate once and grow as triangle counr exceeds allocated
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

bool CNavMeshBuilder::AddOffmeshConnection(const float* pStart, const float* pEnd, float Radius,
										   uchar Dir, uchar Area, ushort Flags)
{
	if (OffMeshCount >= MAX_OFFMESH_CONNECTIONS) FAIL;

	float* v = &OffMeshVerts[OffMeshCount * 3 * 2];
	rcVcopy(&v[0], pStart);
	rcVcopy(&v[3], pEnd);
	OffMeshRadius[OffMeshCount] = Radius;
	OffMeshDir[OffMeshCount] = Dir;
	OffMeshArea[OffMeshCount] = Area;
	OffMeshFlags[OffMeshCount] = Flags;
	OffMeshID[OffMeshCount] = 1000 + OffMeshCount;

	OffMeshCount++;
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

	pCompactHF = rcAllocCompactHeightfield();
	if (!pCompactHF)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		FAIL;
	}

	if (!rcBuildCompactHeightfield(&Ctx, Cfg.walkableHeight, Cfg.walkableClimb, *pHF, *pCompactHF))
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		FAIL;
	}
		
	if (!rcErodeWalkableArea(&Ctx, Cfg.walkableRadius, *pCompactHF))
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CNavMeshBuilder::ApplyConvexVolumeArea()
{
	//const ConvexVolume* vol;
	//rcMarkConvexPolyArea(&Ctx, vols[i].pVerts, vols[i].VertexCount, vols[i].hmin, vols[i].hmax, (uchar)vol->area, *pCompactHF);
}
//---------------------------------------------------------------------

bool CNavMeshBuilder::Build(uchar*& pOutData, int& OutSize, bool MonotonePartitioning)
{
	if (MonotonePartitioning)
	{
		if (!rcBuildRegionsMonotone(&Ctx, *pCompactHF, 0, Cfg.minRegionArea, Cfg.mergeRegionArea))
		{
			Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
			FAIL;
		}
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
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		FAIL;
	}

	n_printf("NavMesh rcBuildContours done\n");
	
	rcPolyMesh* pMesh = rcAllocPolyMesh();
	if (!pMesh)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		FAIL;
	}

	if (!rcBuildPolyMesh(&Ctx, *pContourSet, Cfg.maxVertsPerPoly, *pMesh))
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		FAIL;
	}

	n_printf("NavMesh rcBuildPolyMesh done\n");

	rcPolyMeshDetail* pMeshDetail = rcAllocPolyMeshDetail();
	if (!pMeshDetail)
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		FAIL;
	}

	if (!rcBuildPolyMeshDetail(&Ctx, *pMesh, *pCompactHF, Cfg.detailSampleDist, Cfg.detailSampleMaxError, *pMeshDetail))
	{
		Ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		FAIL;
	}

	n_printf("NavMesh rcBuildPolyMeshDetail done\n");

	rcFreeCompactHeightfield(pCompactHF);
	pCompactHF = NULL;
	rcFreeContourSet(pContourSet);

	// Update poly flags from areas.
	for (int i = 0; i < pMesh->npolys; ++i)
	{
		const ushort NPF_NORMAL = 0x01;
		if (pMesh->areas[i] == RC_WALKABLE_AREA) pMesh->flags[i] = NPF_NORMAL;
	}

	dtNavMeshCreateParams Params;
	memset(&Params, 0, sizeof(Params));
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
	Params.detailMeshes = pMeshDetail->meshes;
	Params.detailVerts = pMeshDetail->verts;
	Params.detailVertsCount = pMeshDetail->nverts;
	Params.detailTris = pMeshDetail->tris;
	Params.detailTriCount = pMeshDetail->ntris;
	Params.offMeshConVerts = OffMeshVerts;
	Params.offMeshConRad = OffMeshRadius;
	Params.offMeshConDir = OffMeshDir;
	Params.offMeshConAreas = OffMeshArea;
	Params.offMeshConFlags = OffMeshFlags;
	Params.offMeshConUserID = OffMeshID;
	Params.offMeshConCount = OffMeshCount;

	if (!dtCreateNavMeshData(&Params, &pOutData, &OutSize))
	{
		Ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
		FAIL;
	}

	n_printf("NavMesh dtCreateNavMeshData done\n");

	//duLogBuildTimes(*Ctx, Ctx.getAccumulatedTime(RC_TIMER_TOTAL));
	Ctx.log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", pMesh->nverts, pMesh->npolys);

	rcFreePolyMesh(pMesh);
	rcFreePolyMeshDetail(pMeshDetail);

	OK;
}
//---------------------------------------------------------------------

void CNavMeshBuilder::Cleanup()
{
	if (pHF)
	{
		rcFreeHeightField(pHF);
		pHF = NULL;
	}
	OffMeshCount = 0;
}
//---------------------------------------------------------------------
