#include "NavSystem.h"

#include <Game/GameLevel.h> //!!!???cache AI level instead of getting from entity every time?!
#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <DetourCommon.h>
#include <DetourLocalBoundary.h>
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

inline void CNavSystem::ResetPathRequest()
{
	if (!pProcessingQueue) return;
	pProcessingQueue->CancelRequest(PathRequestID);
	pProcessingQueue = NULL;
	PathRequestID = DT_PATHQ_INVALID;
}
//---------------------------------------------------------------------

void CNavSystem::Init(const Data::CParams* Params)
{
	if (Params)
	{
		//pNavFilter = AISrv->GetNavQueryFilter(CStrID(Params->Get<CString>(CStrID("NavFilterID"), NULL).CStr()));
		n_error("CNavSystem::Init() -> IMPLEMENT ME!!!");
		
#ifdef DETOUR_OBSTACLE_AVOIDANCE // In ActorFwd.h
		pBoundary = n_new(dtLocalBoundary); //!!!and only if obstacle avoidance enabled!
#else
		pBoundary = NULL;
#endif
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
	pActor->NavState = AINav_IdleInvalid;
	ResetPathRequest();
	SAFE_DELETE(pBoundary);
}
//---------------------------------------------------------------------

void CNavSystem::SetupState()
{
	ReplanTime = 0.f;
	//TopologyOptTime = 0.f;
	OffMeshRef = 0;
	TraversingOffMesh = false;
	ResetPathRequest();

	dtPolyRef Ref = 0;

	pNavQuery = pActor->GetEntity()->GetLevel()->GetAI()->GetSyncNavQuery(pActor->Radius);
	if (pNavQuery)
	{
		const float Extents[3] = { 0.f, pActor->Height, 0.f };
		pNavQuery->findNearestPoly(pActor->Position.v, Extents, pNavFilter, &Ref, NULL);
	}

	pActor->NavState = Ref ? AINav_Done : AINav_IdleInvalid;
	Corridor.reset(Ref, pActor->Position.v);
	if (pBoundary) pBoundary->reset();
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

	UpdateDestination();

	if (DestRef) pActor->NavState = pActor->IsAtValidLocation() ? AINav_DestSet : AINav_Invalid;
	else pActor->NavState = pActor->IsAtValidLocation() ? AINav_Failed : AINav_IdleInvalid;
}
//---------------------------------------------------------------------

void CNavSystem::Reset()
{
	if (pActor->NavState == AINav_Following) EndEdgeTraversal();
	pActor->NavState = pActor->IsAtValidLocation() ? AINav_Done : AINav_IdleInvalid;
	ResetPathRequest();
	Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
	if (pBoundary) pBoundary->reset();
}
//---------------------------------------------------------------------

void CNavSystem::Update(float FrameTime)
{
	ReplanTime += FrameTime;

	if (!pNavQuery) return;

	const float Extents[3] = { 0.f, pActor->Height, 0.f };

	//???request new poly only in UpdatePosition()? if navmesh is immutable, here we can check
	//the first corridor poly as it updated by UpdatePosition()

	// Check if validity of our current location changed
	dtPolyRef Ref;
	pNavQuery->findNearestPoly(pActor->Position.v, Extents, pNavFilter, &Ref, NULL);
	bool NewLocationIsValid = !!Ref;
	if (pActor->IsAtValidLocation() != NewLocationIsValid)
	{
		if (NewLocationIsValid)
		{
			if (pActor->IsNavSystemIdle()) pActor->NavState = AINav_Done;
			else
			{
				pActor->NavState = AINav_DestSet;
				ResetPathRequest();
			}
		}
		else pActor->NavState = pActor->IsNavSystemIdle() ? AINav_IdleInvalid : AINav_Invalid;
		Corridor.reset(Ref, pActor->Position.v);
		if (pBoundary) pBoundary->reset();
	}

	//!!!???if our poly ref changed, trigger some nav poly controller or smth? if we suddenly entered a poly that is
	//blocked by a controller, we can even receive Invalid state fron it here!
	//!!!can update smth like pActor->GroundInfo here!

	if (!NewLocationIsValid) return; //!!!before return can check if at the destination! and return (Done | Invalid).

	// We replan path if any of the following conditions is met:
	// - destination polygon changed, and we aren't at the destination point
	// - path is invalid close ahead
	// - we're following the path, the end is close, and it isn't our destination
	bool Replan = false;

	// Update destination and related state
	if (!pActor->IsNavSystemIdle())
	{
		static const int CHECK_LOOKAHEAD = 10;
		static const float TARGET_REPLAN_DELAY = 1.f; // Seconds

		// If our destination became invalid, we try to find a new one
		if (!pNavQuery->isValidPolyRef(DestRef, pNavFilter))
		{
			UpdateDestination();
			Replan = true;
		}

		if (DestRef)
		{
			// Our destination is valid. Check if we're already there.
			if (pActor->IsAtPoint(DestPoint, true))
			{
				Reset();
				return;
			}

			if (!Corridor.isValid(CHECK_LOOKAHEAD, pNavQuery, pNavFilter))
			{
				// Fix current path
				//Corridor.trimInvalidPath(Corridor.getFirstPoly(), pActor->Position, pNavQuery, pNavFilter);
				//if (pBoundary) pBoundary->reset();
				Replan = true;
			}

			Replan = Replan || (pActor->NavState == AINav_Following &&
								Corridor.getPathCount() < CHECK_LOOKAHEAD &&
								Corridor.getLastPoly() != DestRef &&
								ReplanTime > TARGET_REPLAN_DELAY);

			if (Replan)
			{
				ResetPathRequest();
				pActor->NavState = AINav_DestSet;
			}
		}
		else
		{
			// We have lost our destination
			ResetPathRequest();
			Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
			pActor->NavState = AINav_Failed;
			return;
		}
	}

	if (pActor->NavState == AINav_DestSet)
	{
		n_assert(Corridor.getPathCount() > 0);

		static const int MAX_QUICKSEARCH_ITER = 20;
		static const int MAX_QUICKSEARCH_RES = 32;
		dtPolyRef Path[MAX_QUICKSEARCH_RES];
		int PathSize = 0;

		// Quick seach towards the destination
		pNavQuery->initSlicedFindPath(Corridor.getFirstPoly(), DestRef, pActor->Position.v, DestPoint.v, pNavFilter);
		pNavQuery->updateSlicedFindPath(MAX_QUICKSEARCH_ITER, 0);
		dtStatus QueryStatus = Replan /* && PathSize > CHECK_LOOKAHEAD */ ?
			pNavQuery->finalizeSlicedFindPathPartial(Corridor.getPath(), Corridor.getPathCount(), Path, &PathSize, MAX_QUICKSEARCH_RES) :
			pNavQuery->finalizeSlicedFindPath(Path, &PathSize, MAX_QUICKSEARCH_RES);

		// Calculate target, which is the closest position to DestPoint inside a quick path
		float TargetPos[3];
		if (!dtStatusFailed(QueryStatus) && PathSize > 0)
		{
			// In progress or succeed
			if (Path[PathSize - 1] != DestRef)
			{
				// Partial path, constrain target position inside the last polygon
				QueryStatus = pNavQuery->closestPointOnPoly(Path[PathSize - 1], DestPoint.v, TargetPos);
				if (dtStatusFailed(QueryStatus)) PathSize = 0;
			}
			else dtVcopy(TargetPos, DestPoint.v);
		}
		else PathSize = 0;
			
		if (!PathSize)
		{
			// Could not find quick path, start the full request from the current location
			dtVcopy(TargetPos, pActor->Position.v);
			Path[0] = Corridor.getFirstPoly();
			PathSize = 1;
		}

		// Set our quick path to the corridor. If this path is partial, the full request will be
		// issued from the end of a quick path to the destination in a AINav_Planning state.
		Corridor.setCorridor(TargetPos, Path, PathSize);
		if (pBoundary) pBoundary->reset();

		if (Path[PathSize - 1] == DestRef)
		{
			pActor->NavState = AINav_Following;
			ReplanTime = 0.f;
		}
		else pActor->NavState = AINav_Planning;
	}

	if (pActor->NavState == AINav_Planning)
	{
		if (PathRequestID == DT_PATHQ_INVALID)
		{
			dtNavMeshQuery* pAsyncQuery;
			if (pActor->GetEntity()->GetLevel()->GetAI()->GetAsyncNavQuery(pActor->Radius, pAsyncQuery, pProcessingQueue))
			{
				//!!!Insert request based on greatest targetReplanTime!
				PathRequestID = pProcessingQueue->Request(Corridor.getLastPoly(), DestRef, Corridor.getTarget(),
									DestPoint.v, pAsyncQuery, pNavFilter);
			}
		}
		else
		{
			n_assert_dbg(pProcessingQueue);

			dtStatus QueryStatus = pProcessingQueue->GetRequestStatus(PathRequestID);
			if (dtStatusFailed(QueryStatus))
			{
				pProcessingQueue = NULL;
				PathRequestID = DT_PATHQ_INVALID;
				pActor->NavState = AINav_Failed;
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

					//???can upload the path to the corridor manually? may be more optimal!

					// There was a quick path, merge it with full path returned
					if (OldPathSize > 1)
					{
						//!!!!!!!!!???remove trackbacks before trimming?!!
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

					// Calculate new target position, since path may be partial
					float TargetPoint[3];
					bool Valid = true;

					if (Path[PathSize - 1] != DestRef)
						Valid = dtStatusSucceed(pNavQuery->closestPointOnPoly(Path[PathSize - 1], DestPoint.v, TargetPoint));
					else dtVcopy(TargetPoint, DestPoint.v);

					if (Valid)
					{
						// Set new path to the corridor and follow it until replanning is needed
						Corridor.setCorridor(TargetPoint, Path, PathSize);
						if (pBoundary) pBoundary->reset();
						pActor->NavState = AINav_Following;
					}
					else pActor->NavState = AINav_Failed;
				}
				else pActor->NavState = AINav_Failed;

				ReplanTime = 0.f;
				pProcessingQueue = NULL;
				PathRequestID = DT_PATHQ_INVALID;
			}
		}
	}

	// The only action in a AINav_Following state for now is a periodic path topology optimization
	bool OptimizePathTopology = true; //!!!to settings!
	if (pActor->NavState == AINav_Following && OptimizePathTopology)
	{
		const float OPT_TIME_THR = 0.5f; // seconds
		//nqueue = addToOptQueue(ag, queue, nqueue, OPT_MAX_AGENTS); - global (not per-actor) task queue!
		TopologyOptTime += FrameTime;
		if (TopologyOptTime >= OPT_TIME_THR)
		{
			Corridor.optimizePathTopology(pNavQuery, pNavFilter);
			TopologyOptTime = 0.f;
		}
	}
}
//---------------------------------------------------------------------

// Updates navigation state as actor position changes, either externally or by navigation-driven movement
void CNavSystem::UpdatePosition()
{
	if (!pNavQuery || TraversingOffMesh) return;

	if (OffMeshRef && OffMeshPoint.lensquared() <= OffMeshRadius)
	{
		// We reached an enter to the offmesh connection that is the next edge of our path (see GetPathEdges())
		//???check validity of the offmesh poly ref? what if we're at the invalid pos now?
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
		// We walk or teleport over the level

		n_error("CNavSystem::UpdatePosition() -> IMPLEMENT ME!!!");

		//if (!pActor->IsAtValidLocation()) return;

		//!!!???what to do if was invalid and now invalid/valid?
		//etc
		//???unify SetState, Update & this logic

		if (pActor->IsNavSystemIdle())
		{
			// find poly at the new pos
			// if not found, set invalid
			// if found, state = Done
		}
		else
		{
			Corridor.movePosition(pActor->Position.v, pNavQuery, pNavFilter);

			// The resulting position will differ from the desired position if the desired position
			// is not on the navigation mesh, or it can't be reached using a local search
			// (c) Detour docs
			if (pActor->Position.x == Corridor.getPos()[0] &&
				pActor->Position.z == Corridor.getPos()[2] &&
				pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter))
			{
				// Found
				Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
			}
			else
			{
				// find poly at the new pos
				// if not found, set invalid
				// if found, state = DestSet to replan path to dest
			}

			// if valid, update destination to the navmesh
		}

		// The resulting position will differ from the desired position if the desired position
		// is not on the navigation mesh, or it can't be reached using a local search
		// (c) Detour docs
		if (pActor->Position.x == Corridor.getPos()[0] &&
			pActor->Position.z == Corridor.getPos()[2] &&
			pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter))
		{
			if (pActor->NavState == AINav_Done)
				Corridor.reset(Corridor.getFirstPoly(), pActor->Position.v);
		}
		else
		{
			//!!!try to reset position from the scratch!
			//if failed set invalid

			pActor->NavState = AINav_Invalid;
			Corridor.reset(0, pActor->Position.v);
			if (pBoundary) pBoundary->reset();
		}

		pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
	}
}
//---------------------------------------------------------------------

void CNavSystem::UpdateDestination()
{
	//!!!'StrictDest' flag - if DestPoint != Nearest (with very low tolerance) fail to set dest!
	//???!!!when can call Corridor.moveTargetPosition?!
	const float Extents[3] = { 0.f, pActor->Height, 0.f };
	float Nearest[3];
	pNavQuery->findNearestPoly(DestPoint.v, Extents, pNavFilter, &DestRef, Nearest);
	dtVcopy(DestPoint.v, Nearest);
	pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
}
//---------------------------------------------------------------------

// We take path edges to the external actions, like steering, climbing, jumping, sweeming, door traversal etc.
// When external action finishes, it must inform the navigation system that it ended with its edge.
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

bool CNavSystem::GetPathEdges(CArray<CPathEdge>& OutPath, int MaxSize)
{
	n_assert(MaxSize > 0);

	if (!pNavQuery || pActor->NavState != AINav_Following) FAIL;

	const dtNavMesh* pNavMesh = pNavQuery->getAttachedNavMesh();

	OutPath.Clear();

	// If we are at the offmesh connection, add it as the first edge
	if (TraversingOffMesh)
	{
		CPathEdge* pEdge = OutPath.Reserve(1);
		pEdge->Action = GetPolyAction(pNavMesh, OffMeshRef);
		pEdge->Dest = OffMeshPoint;
		pEdge->IsLast = false; //???can offmesh connection be the last edge?
	}

	if (OutPath.GetCount() >= MaxSize) OK;

	// Make buffers larger if more edges are required, can limit MaxSize value and set MAX_CORNERS = MaxMaxSize + 1
	const int MAX_CORNERS = 3;
	float CornerVerts[MAX_CORNERS * 3];
	unsigned char CornerFlags[MAX_CORNERS];
	dtPolyRef CornerPolys[MAX_CORNERS];
	int CornerCount = Corridor.findCorners(CornerVerts, CornerFlags, CornerPolys, MAX_CORNERS, pNavQuery, pNavFilter);

	if (!CornerCount) OK;

	//!!!Optimize not every frame but only when needed to fix path!
	// (adjust call frequenCY, and mb force when path changes). More frequent when avoid obstacles
	const float* pNextCorner = &CornerVerts[dtMin(1, CornerCount - 1) * 3];
	bool OptimizePathVis = true; //!!!to settings!
	if (OptimizePathVis)
		Corridor.optimizePathVisibility(pNextCorner, 30.f * pActor->Radius, pNavQuery, pNavFilter);

	// Here we build path edges from corner points returned by the corridor.
	// Path edge starts at the end of prev. edge (or at the current position) and ends at the
	// next corner or at the action change point. Action change point lies at the intersection of
	// the path edge and an edge shared between two path polygons with different traversal actions.
	// This guarantees that one edge has exactly one traversal action, and no action is skipped.
	const dtPolyRef* pCurrPoly = Corridor.getPath();
	const dtPolyRef* pLastPoly = Corridor.getPath() + Corridor.getPathCount();
	CPathEdge* pEdge;
	for (int i = 0; (i < CornerCount) && (OutPath.GetCount() < MaxSize); ++i)
	{
		pEdge = OutPath.Reserve(1);
		pEdge->Action = CStrID::Empty;
		pEdge->IsLast = false;

		do
		{
			CStrID Action = GetPolyAction(pNavMesh, *pCurrPoly);
			if (pEdge->Action == CStrID::Empty) pEdge->Action = Action;
			else if (pEdge->Action != Action)
			{
				// AFAIR Detour has related feature implemented in findStraightPath() in the last revision.
				// So can explore how they determine line-navpoly intersection.
				n_error("IMPLEMENT ME!!! Navigation action change point");

				float Intersection[3]; //!!!!!
				pEdge->Dest.set(Intersection);
				//???edge destination must have height of NavMesh or of actual geometry? Steering may depend on it.
				//pNavQuery->getPolyHeight(*pCurrPoly, Intersection, &pNode->Dest.y);

				if (OutPath.GetCount() >= MaxSize) OK;

				CPathEdge* pEdge = OutPath.Reserve(1);
				pEdge->Action = Action;
				pEdge->IsLast = false;
			}

			if (*pCurrPoly == CornerPolys[i]) break;
		}
		while (++pCurrPoly < pLastPoly);

		pEdge->Dest.set(&CornerVerts[i * 3]);
		//???edge destination must have height of NavMesh or of actual geometry? Steering may depend on it.
		//pNavQuery->getPolyHeight(CornerPolys[i] ? CornerPolys[i] : DestRef, &CornerVerts[i * 3], &pNode->Dest.y);
	}

	pEdge->IsLast = !!(CornerFlags[CornerCount - 1] & DT_STRAIGHTPATH_END);

	//!!!when add node, determine navigation controller for the poly
	//if action not changed, but controller has changed, finalize edge and start new one
	//!!!if poly has controller, can determine action from a controller

	// If the last corner is an offmesh connection start, we record its info and
	// UpdatePosition() will detect if we are at the trigger radius to start traversal
	if (CornerFlags[CornerCount - 1] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
	{
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
	if (!pNavQuery || !pBoundary) return;

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
	if (pActor->NavState == AINav_Invalid) FAIL;

	if (!pNavQuery) FAIL;

	//!!!Need to clamp to radius!
	dtPolyRef Ref;
	return dtStatusSucceed(pNavQuery->findRandomPointAroundCircle(Corridor.getFirstPoly(), pActor->Position.v, Range,
		pNavFilter, n_rand, &Ref, Location.v));
}
//---------------------------------------------------------------------
*/

}