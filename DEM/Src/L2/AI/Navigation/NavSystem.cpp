#include "NavSystem.h"

#include <Game/GameLevel.h> //!!!???cache AI level instead of getting from entity every time?!
#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <DetourCommon.h>
#include <DetourObstacleAvoidance.h>

namespace AI
{

CNavSystem::CNavSystem(CActor* Actor):
	pActor(Actor),
	pNavQuery(NULL),
	pNavFilter(NULL),
	pBoundary(NULL),
	DestRef(0),
	OffMeshRef(0),
	pProcessingQueue(NULL),
	PathRequestID(DT_PATHQ_INVALID),
	TraversingOffMesh(false)
{
	Corridor.init(MAX_NAV_PATH);
}
//---------------------------------------------------------------------

void CNavSystem::Init(const Data::CParams* Params)
{
	if (Params)
	{
		//pNavFilter = AISrv->GetNavQueryFilter(CStrID(Params->Get<CString>(CStrID("NavFilterID"), NULL).CStr()));
		
		//!!!if avoid obstacles
		pBoundary = n_new(dtLocalBoundary);
	}
	else
	{
		pNavFilter = AISrv->GetDefaultNavQueryFilter();
		pBoundary = NULL;
	}

	//!!!DBG TMP!
	EdgeTypeToAction.Add(0, CStrID("SteerToPosition"));

	//DestPoint = pActor->Position; //???restore from attrs on load?
	//DestRef = 0;

	SetupState();
}
//---------------------------------------------------------------------

void CNavSystem::Term()
{
	pActor->NavStatus = AINav_Invalid;
	if (pProcessingQueue)
	{
		pProcessingQueue->CancelRequest(PathRequestID);
		pProcessingQueue = NULL;
		PathRequestID = DT_PATHQ_INVALID;
	}
	n_delete(pBoundary);
}
//---------------------------------------------------------------------

void CNavSystem::SetupState()
{
	ReplaCTime = 0.f;
	//TopologyOptTime = 0.f;
	OffMeshRef = 0;
	TraversingOffMesh = false;
	if (pProcessingQueue)
	{
		pProcessingQueue->CancelRequest(PathRequestID);
		pProcessingQueue = NULL;
		PathRequestID = DT_PATHQ_INVALID;
	}

	dtPolyRef Ref = 0;

	pNavQuery = pActor->GetEntity()->GetLevel()->GetAI()->GetSyncNavQuery(pActor->Radius);
	if (pNavQuery)
	{
		const float Extents[3] = { 0.f, pActor->Height, 0.f };
		pNavQuery->findNearestPoly(pActor->Position.v, Extents, pNavFilter, &Ref, NULL);
	}

	pActor->NavStatus = Ref ? AINav_Done : AINav_Invalid;
	Corridor.reset(Ref, pActor->Position.v);

	if (pBoundary) pBoundary->reset();
}
//---------------------------------------------------------------------

void CNavSystem::Update(float FrameTime)
{
	ReplaCTime += FrameTime;

	if (!pNavQuery) return;

	const float Extents[3] = { 0.f, pActor->Height, 0.f };

	if (pActor->NavStatus == AINav_Invalid)
	{
		dtPolyRef Ref;
		pNavQuery->findNearestPoly(pActor->Position.v, Extents, pNavFilter, &Ref, NULL);

		if (Ref)
		{
			pActor->NavStatus = AINav_Done;
			Corridor.reset(Ref, pActor->Position.v);
			if (pBoundary) pBoundary->reset();
		}
	}

	bool Replan = false;

	if (pActor->NavStatus != AINav_Invalid)
	{
		if (pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter))
		{
			if (pActor->NavStatus == AINav_Done)
				Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
		}
		else
		{
			pActor->NavStatus = AINav_Invalid;
			Corridor.reset(0, pActor->Position.v);
			if (pBoundary) pBoundary->reset();
		}
	}

	if (pActor->NavStatus != AINav_Done &&
		pActor->NavStatus != AINav_Failed &&
		pActor->NavStatus != AINav_Invalid)
	{
		static const int CHECK_LOOKAHEAD = 10;
		static const float TARGET_REPLAN_DELAY = 1.f; // seconds

		if (!pNavQuery->isValidPolyRef(DestRef, pNavFilter))
		{
			float Nearest[3];
			pNavQuery->findNearestPoly(DestPoint.v, Extents, pNavFilter, &DestRef, Nearest);
			dtVcopy(DestPoint.v, Nearest);
			pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
			Replan = true; //???can just move target into the corridor?
		}

		if (DestRef)
		{
			if (!Corridor.isValid(CHECK_LOOKAHEAD, pNavQuery, pNavFilter))
			{
				// Fix current path.
				//Corridor.trimInvalidPath(Corridor.getFirstPoly(), pActor->Position, pNavQuery, pNavFilter);
				//if (pBoundary) pBoundary->reset();
				Replan = true;
			}

			Replan = Replan || (pActor->NavStatus == AINav_Following &&
								ReplaCTime > TARGET_REPLAN_DELAY &&
								Corridor.getPathCount() < CHECK_LOOKAHEAD &&
								Corridor.getLastPoly() != DestRef);

			if (Replan)
			{
				pProcessingQueue = NULL;
				PathRequestID = DT_PATHQ_INVALID;
				pActor->NavStatus = DestRef ? AINav_DestSet : AINav_Failed;
			}
		}
		else
		{
			Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
			pActor->NavStatus = AINav_Done;
			Replan = false;
		}
	}

	if (pActor->NavStatus == AINav_DestSet)
	{
		n_assert(Corridor.getPathCount() > 0);

		static const int MAX_RES = 32;
		dtPolyRef Path[MAX_RES];
		int PathSize = 0;

		// Quick seach towards the goal.
		static const int MAX_ITER = 20;
		pNavQuery->initSlicedFindPath(Corridor.getFirstPoly(), DestRef, pActor->Position.v, DestPoint.v, pNavFilter);
		pNavQuery->updateSlicedFindPath(MAX_ITER, 0);
		dtStatus QueryStatus = 0;
		if (Replan) // && npath > 10)
			QueryStatus = pNavQuery->finalizeSlicedFindPathPartial(Corridor.getPath(), Corridor.getPathCount(), Path, &PathSize, MAX_RES);
		else
			QueryStatus = pNavQuery->finalizeSlicedFindPath(Path, &PathSize, MAX_RES);

		float TargetPos[3];
		if (!dtStatusFailed(QueryStatus) && PathSize > 0)
		{
			// In progress or succeed.
			if (Path[PathSize - 1] != DestRef)
			{
				// Partial path, constrain target position inside the last polygon.
				QueryStatus = pNavQuery->closestPointOnPoly(Path[PathSize-1], DestPoint.v, TargetPos);
				if (dtStatusFailed(QueryStatus)) PathSize = 0;
			}
			else dtVcopy(TargetPos, DestPoint.v);
		}
		else PathSize = 0;
			
		if (!PathSize)
		{
			// Could not find path, start the request from current location.
			dtVcopy(TargetPos, pActor->Position.v);
			Path[0] = Corridor.getFirstPoly();
			PathSize = 1;
		}

		Corridor.setCorridor(TargetPos, Path, PathSize);
		if (pBoundary) pBoundary->reset();

		if (Path[PathSize-1] == DestRef)
		{
			pActor->NavStatus = AINav_Following;
			ReplaCTime = 0.f;
		}
		else pActor->NavStatus = AINav_Planning;
	}
	
	if (pActor->NavStatus == AINav_Planning)
	{
		if (PathRequestID == DT_PATHQ_INVALID)
		{
			dtNavMeshQuery* pAsyncQuery;
			if (pActor->GetEntity()->GetLevel()->GetAI()->GetAsyncNavQuery(pActor->Radius, pAsyncQuery, pProcessingQueue))
			{
				//!!!Insert request based on greatest targetReplaCTime!
				PathRequestID = pProcessingQueue->Request(Corridor.getLastPoly(), DestRef, Corridor.getTarget(),
									DestPoint.v, pAsyncQuery, pNavFilter);
			}
			//else if (true) // bool FallbackToStraightPath
			//{
			//	CurrPath.Clear();
			//	CPathEdge* pNode = CurrPath.Reserve(1);
			//	pNode->Dest = DestPoint;
			//	pActor->NavStatus = AINav_PathBuilt;
			//}
		}
		else
		{
			n_assert(pProcessingQueue);

			dtStatus QueryStatus = pProcessingQueue->GetRequestStatus(PathRequestID);
			if (dtStatusFailed(QueryStatus))
			{
				pProcessingQueue = NULL;
				PathRequestID = DT_PATHQ_INVALID;
				pActor->NavStatus = DestRef ? AINav_DestSet : AINav_Failed;
			}
			else if (dtStatusSucceed(QueryStatus))
			{
				n_assert(Corridor.getPathCount() > 0);
				
				dtPolyRef Path[MAX_NAV_PATH];
				int PathSize = 0;
				QueryStatus = pProcessingQueue->GetPathResult(PathRequestID, Path, PathSize, MAX_NAV_PATH);

				if (!dtStatusFailed(QueryStatus) && PathSize && Corridor.getLastPoly() == Path[0])
				{
					const int OldPathSize = Corridor.getPathCount();
					if (OldPathSize > 1)
					{
						if ((OldPathSize - 1) + PathSize > MAX_NAV_PATH)
							PathSize = MAX_NAV_PATH - (OldPathSize - 1);
						
						memmove(Path + OldPathSize - 1, Path, sizeof(dtPolyRef) * PathSize);
						memcpy(Path, Corridor.getPath(), sizeof(dtPolyRef) * (OldPathSize - 1));
						PathSize += OldPathSize - 1;
						
						// Remove trackbacks
						for (int j = 0; j < PathSize; ++j)
							if (j - 1 >= 0 && j + 1 < PathSize)
								if (Path[j - 1] == Path[j + 1])
								{
									memmove(Path + (j - 1), Path + (j + 1), sizeof(dtPolyRef) * (PathSize - (j + 1)));
									PathSize -= 2;
									j -= 2;
								}						
					}
					
					float TargetPoint[3];
					bool Valid = true;

					// Check for partial path.
					if (Path[PathSize - 1] != DestRef)
						Valid = dtStatusSucceed(pNavQuery->closestPointOnPoly(Path[PathSize - 1], DestPoint.v, TargetPoint));
					else dtVcopy(TargetPoint, DestPoint.v);

					if (Valid)
					{
						Corridor.setCorridor(TargetPoint, Path, PathSize);
						if (pBoundary) pBoundary->reset();
						pActor->NavStatus = AINav_Following;
					}
					else pActor->NavStatus = AINav_Failed;
				}

				ReplaCTime = 0.f;
				pProcessingQueue = NULL;
				PathRequestID = DT_PATHQ_INVALID;
			}
		}
	}

	if (pActor->NavStatus == AINav_Following)
	{
		bool OptimizePathTopology = true; //!!!to settings!
		if (OptimizePathTopology)
		{
			const float OPT_TIME_THR = 0.5f; // seconds
			//nqueue = addToOptQueue(ag, queue, nqueue, OPT_MAX_AGENTS);
			TopologyOptTime += FrameTime;
			if (TopologyOptTime >= OPT_TIME_THR)
			{
				Corridor.optimizePathTopology(pNavQuery, pNavFilter);
				TopologyOptTime = 0.f;
			}
		}

		//???here or in action?
		//GetPathEdges(Path, 2);
	}

	if (pActor->NavStatus != AINav_Done &&
		pActor->NavStatus != AINav_Failed &&
		pActor->NavStatus != AINav_Invalid)
	{
		if (pActor->IsAtPoint(DestPoint, true)) Reset();
	}
}
//---------------------------------------------------------------------

CStrID CNavSystem::GetPolyAction(const dtNavMesh* pNavMesh, dtPolyRef Ref)
{
	const dtMeshTile* pTile;
	const dtPoly* pPoly;
	pNavMesh->getTileAndPolyByRefUnsafe(Ref, &pTile, &pPoly);
	uchar PolyArea = pPoly->getArea();
	ushort PolyFlags = pPoly->flags;

	//!!!DBG TMP!
	int EdgeType = -1; //!!!Some calcs from ?PolyArea? and PolyFlags!
	//if (PolyFlags == 0x01 && (PolyArea == 63 || PolyArea == 1))
	{
		// Normal poly
		EdgeType = 0;
	}

	//int Idx = EdgeTypeToAction.FindIndex(EdgeType);
	//return Idx == INVALID_INDEX ? CStrID::Empty : EdgeTypeToAction.ValueAt(Idx);

	return EdgeTypeToAction[EdgeType];
}
//---------------------------------------------------------------------

void CNavSystem::Reset()
{
	if (pActor->NavStatus == AINav_Following) EndEdgeTraversal();
	if (pActor->NavStatus != AINav_Invalid) pActor->NavStatus = AINav_Done;	
	if (pProcessingQueue)
	{
		pProcessingQueue->CancelRequest(PathRequestID);
		pProcessingQueue = NULL;
		PathRequestID = DT_PATHQ_INVALID;
	}
	Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
	if (pBoundary) pBoundary->reset();
}
//---------------------------------------------------------------------

void CNavSystem::UpdatePosition()
{
	if (TraversingOffMesh || pActor->NavStatus == AINav_Invalid) return;

	if (!pNavQuery) return;

	if (OffMeshRef && OffMeshPoint.lensquared() <= OffMeshRadius)
	{
		dtPolyRef Refs[2];
		float Dummy[3];
		if (Corridor.moveOverOffmeshConnection(OffMeshRef, Refs, Dummy, OffMeshPoint.v, pNavQuery))
		{
			n_assert(OffMeshRef == Refs[1]);
			TraversingOffMesh = true;
		}
		else n_error("CNavSystem::UpdatePosition() -> Entering invalid offmesh connection");
	}
	else
	{
		Corridor.movePosition(pActor->Position.v, pNavQuery, pNavFilter);

		if (pActor->Position.x == Corridor.getPos()[0] &&
			pActor->Position.z == Corridor.getPos()[2] &&
			pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter))
		{
			if (pActor->NavStatus == AINav_Done)
				Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
		}
		else
		{
			pActor->NavStatus = AINav_Invalid;
			Corridor.reset(0, pActor->Position.v);
			if (pBoundary) pBoundary->reset();
		}

		pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
	}
}
//---------------------------------------------------------------------

void CNavSystem::SetDestPoint(const vector3& Dest)
{
	DestPoint = Dest;

	if (pActor->IsAtPoint(DestPoint, true))
	{
		Reset();
		return;
	}

	if (!pNavQuery) return;

	//???!!!when can call Corridor.moveTargetPosition?!

	float Nearest[3];
	const float Extents[3] = { 0.f, pActor->Height, 0.f };
	pNavQuery->findNearestPoly(DestPoint.v, Extents, pNavFilter, &DestRef, Nearest);
	dtVcopy(DestPoint.v, Nearest);

	//???allow partial path (to the navmesh edge)? extend Extents, if so!

	pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);

	//!!!NEED TWO STATES - VALIDITY & DEST! Can have dest and be invalid at a time
	if (pActor->NavStatus != AINav_Invalid)
		pActor->NavStatus = AINav_DestSet;
}
//---------------------------------------------------------------------

void CNavSystem::EndEdgeTraversal()
{
	if (TraversingOffMesh)
	{
		OffMeshRef = 0;
		TraversingOffMesh = false;
		UpdatePosition(); //???need or always called from other place?
	}

	//???process last edge traversal?
}
//---------------------------------------------------------------------

bool CNavSystem::GetPathEdges(CArray<CPathEdge>& OutPath, int MaxSize)
{
	n_assert(MaxSize > 0);

	if (pActor->NavStatus != AINav_Following) FAIL;

	if (!pNavQuery) FAIL;

	const dtNavMesh* pNavMesh = pNavQuery->getAttachedNavMesh();

	OutPath.Clear();

	if (TraversingOffMesh)
	{
		CPathEdge* pNode = OutPath.Reserve(1);
		pNode->Action = GetPolyAction(pNavMesh, OffMeshRef);
		pNode->Dest = OffMeshPoint;
		pNode->IsLast = false; //???can offmesh connection be the last edge?
	}

	if (OutPath.GetCount() >= MaxSize) OK;

	// Make buffers larger if more edges are required, can limit MaxSize value and set MAX_CORNERS = MaxMaxSize + 1
	const int MAX_CORNERS = 3;
	float CornerVerts[MAX_CORNERS * 3];
	unsigned char CornerFlags[MAX_CORNERS];
	dtPolyRef CornerPolys[MAX_CORNERS];
	int CornerCount = Corridor.findCorners(CornerVerts, CornerFlags, CornerPolys, MAX_CORNERS, pNavQuery, pNavFilter);

	if (!CornerCount) OK;

	//!!!Optimize not every frame but only when needed to fix path! (adjust call frequency)
	// More frequent when avoid obstacles
	const float* pNextCorner = &CornerVerts[dtMin(1, CornerCount - 1) * 3];
	bool OptimizePathVis = true; //!!!to settings!
	if (OptimizePathVis)
		Corridor.optimizePathVisibility(pNextCorner, 30.f * pActor->Radius, pNavQuery, pNavFilter);

	const dtPolyRef* pCurrPoly = Corridor.getPath();
	const dtPolyRef* pLastPoly = Corridor.getPath() + Corridor.getPathCount();
	CPathEdge* pNode;
	int i;
	for (i = 0; (i < CornerCount) && (OutPath.GetCount() < MaxSize); ++i)
	{
		pNode = OutPath.Reserve(1);
		pNode->Action = CStrID::Empty;
		pNode->IsLast = false;

		do
		{
			CStrID Action = GetPolyAction(pNavMesh, *pCurrPoly);
			if (pNode->Action == CStrID::Empty) pNode->Action = Action;
			else if (pNode->Action != Action)
			{
				float Intersection[3]; //!!!!!
				pNode->Dest.set(Intersection);
				//pNavQuery->getPolyHeight(*pCurrPoly, Intersection, &pNode->Dest.y);
				//need to build correct navmesh
				//or get height from physics
				//or check xz distance & height difference instead of 3D distance (or 2D with equal height)

				if (OutPath.GetCount() >= MaxSize) OK;

				CPathEdge* pNode = OutPath.Reserve(1);
				pNode->Action = Action;
				pNode->IsLast = false;
			}

			if (*pCurrPoly == CornerPolys[i]) break;
		}
		while (++pCurrPoly < pLastPoly);

		pNode->Dest.set(&CornerVerts[i * 3]);
		//pNavQuery->getPolyHeight(CornerPolys[i] ? CornerPolys[i] : DestRef, &CornerVerts[i * 3], &pNode->Dest.y);
		//need to build correct navmesh
		//or get height from physics
		//or check xz distance & height difference instead of 3D distance (or 2D with equal height)
	}

	pNode->IsLast = !!(CornerFlags[CornerCount - 1] & DT_STRAIGHTPATH_END);

	//!!!when add node, determine navigation controller for the poly
	//if action not changed, but controller has changed, finalize edge and start new one
	//!!!if poly has controller, can determine action from a controller

	if (CornerFlags[CornerCount - 1] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
	{
		// If last edge has offmesh connection point as destination, expect offmesh traversal
		OffMeshRef = CornerPolys[CornerCount - 1];
		OffMeshPoint.set(&CornerVerts[(CornerCount - 1) * 3]);
		OffMeshRadius = pActor->Radius * 1.2f; //!!!DetermineOffMeshConnectionTriggerRadius();
	}
	else OffMeshRef = 0;

	OK;
}
//---------------------------------------------------------------------

void CNavSystem::GetObstacles(float Range, dtObstacleAvoidanceQuery& Query)
{
	if (!pBoundary) return;

	if (!pNavQuery) return;

	//!!!check threshold & range values!
	float SqMaxOffset = dtSqr(pActor->Radius * 12.f * 0.25f);
	if (dtVdist2DSqr(pActor->Position.v, pBoundary->getCenter()) > SqMaxOffset || !pBoundary->isValid(pNavQuery, pNavFilter))
		pBoundary->update(Corridor.getFirstPoly(), pActor->Position.v, Range, pNavQuery, pNavFilter);

	for (int j = 0; j < pBoundary->getSegmentCount(); ++j)
	{
		const float* pSeg = pBoundary->getSegment(j);
		if (dtTriArea2D(pActor->Position.v, pSeg, pSeg + 3) >= 0.f)
			Query.addSegment(pSeg, pSeg + 3);
	}
}
//---------------------------------------------------------------------

/*
bool CNavSystem::GetRandomValidLocation(float Range, vector3& Location)
{
	if (pActor->NavStatus == AINav_Invalid) FAIL;

	if (!pNavQuery) FAIL;

	//!!!Need to clamp to radius!
	dtPolyRef Ref;
	return dtStatusSucceed(pNavQuery->findRandomPointAroundCircle(Corridor.getFirstPoly(), pActor->Position.v, Range,
		pNavFilter, n_rand, &Ref, Location.v));
}
//---------------------------------------------------------------------
*/

} //namespace AI