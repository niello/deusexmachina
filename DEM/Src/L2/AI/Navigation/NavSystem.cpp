#include "NavSystem.h"

#include <Game/GameLevel.h>
#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <Math/Math.h>
#include <Render/DebugDraw.h>
#include <DetourCommon.h>
#include <DetourLocalBoundary.h>
#include <DetourObstacleAvoidance.h>
#include <DetourDebugDraw.h>

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
		Core::Error("CNavSystem::Init() -> IMPLEMENT ME!!!");
		
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

	SetupState();
}
//---------------------------------------------------------------------

void CNavSystem::Term()
{
	pActor->NavState = AINav_Done;
	pActor->SetNavLocationValid(false);
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
	pNavQuery = pActor->GetEntity()->GetLevel()->GetAI()->GetSyncNavQuery(pActor->Radius);

	ResetPositionPoly(true);
	if (!pActor->IsNavSystemIdle()) ResetDestinationPoly();
}
//---------------------------------------------------------------------

void CNavSystem::SetDestPoint(const vector3& Dest)
{
	//???OFM what if traversing offmesh?

	if (Dest == DestPoint && !pActor->IsNavSystemIdle()) return;

	DestPoint = Dest;

	if (pActor->IsAtPoint(DestPoint, true))
	{
		Reset(true);
		return;
	}

	if (!pNavQuery) return;

	bool ResetPoly;

	if (pActor->NavState == AINav_Following)
	{
		// In AINav_Following corridor target is the destination, try to update it through the corridor.
		// In other states corridor target is unset or is a partial path target, so replanning is needed.
		Corridor.moveTargetPosition(DestPoint.v, pNavQuery, pNavFilter);
		ResetPoly = DestPoint.x != Corridor.getTarget()[0] || DestPoint.z != Corridor.getTarget()[2];
	}
	else ResetPoly = true;

	if (ResetPoly)
	{
		ResetDestinationPoly();
		if (DestRef && !pActor->IsNavLocationValid())
			ResetPositionPoly(false);
	}
}
//---------------------------------------------------------------------

void CNavSystem::Reset(bool Success)
{
	if (pActor->NavState == AINav_Following) EndEdgeTraversal();
	pActor->NavState = Success ? AINav_Done : AINav_Failed;
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

//!!!OFM need offmesh logic!
	// Planning must work, following may be too, but if there is only topology optimization, it is not necessary
	// Checking if we're at the dest must not work
	// What if path became invalid in the middle of an offmesh connection? Break or finish traversal?
	// Need some function of the offmesh - can break traversal. Say, ladder climbing may return true, but jumping may not
	// Some traversals may want to be completed at the action level (climbing tries to be completed not to leave
	// character on the ladder, if ladder is still valid and path broken farther ahead)
	// If offmesh, validity check must be performed against its ref!
//end OFM

	bool PosIsValidNow =
		pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter) &&
		pActor->Position.x == Corridor.getPos()[0] &&
		pActor->Position.z == Corridor.getPos()[2];
	pActor->SetNavLocationValid(PosIsValidNow);

	if (pActor->IsNavSystemIdle()) return;

	bool Replan = false;

	// Update destination and related state
	static const int CHECK_LOOKAHEAD = 10;
	static const float TARGET_REPLAN_DELAY = 1.f; // Seconds

	// If our destination became invalid, we try to find a new one and replan path to reach it
	if (!pNavQuery->isValidPolyRef(DestRef, pNavFilter))
	{
		ResetDestinationPoly();
		if (pActor->IsNavSystemIdle()) return;
		Replan = true;
	}

	if (!Corridor.isValid(CHECK_LOOKAHEAD, pNavQuery, pNavFilter))
	{
		// Fix current path
		//Corridor.trimInvalidPath(Corridor.getFirstPoly(), pActor->Position, pNavQuery, pNavFilter);
		//if (pBoundary) pBoundary->reset();
		Replan = true;
	}

	// We're following the path, the end is close, and it isn't our destination
	Replan = Replan || (pActor->NavState == AINav_Following &&
						pActor->IsNavLocationValid() &&
						Corridor.getPathCount() < CHECK_LOOKAHEAD &&
						Corridor.getLastPoly() != DestRef &&
						ReplanTime > TARGET_REPLAN_DELAY);

	if (Replan) pActor->NavState = AINav_DestSet;

	if (pActor->NavState == AINav_DestSet)
	{
		n_assert(Corridor.getPathCount() > 0);

		ResetPathRequest();

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
					const int OldPathSize = Corridor.getPathCount() - 1;

					// There was a quick path, merge it with full path returned
					if (OldPathSize)
					{
						if (PathSize > MAX_NAV_PATH - OldPathSize)
							PathSize = MAX_NAV_PATH - OldPathSize;

						memmove(Path + OldPathSize, Path, sizeof(dtPolyRef) * PathSize);
						memcpy(Path, Corridor.getPath(), sizeof(dtPolyRef) * OldPathSize);
						PathSize += OldPathSize;

						// Remove trackbacks
						for (int i = 1; i < PathSize - 1; ++i)
							if (Path[i - 1] == Path[i + 1])
							{
								memmove(Path + i - 1, Path + i + 1, sizeof(dtPolyRef) * (PathSize - (i + 1)));
								PathSize -= 2;
								i = n_min(i - 2, 1);
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

	// The only action in a AINav_Following state for now is a periodic path topology optimization.
	// It is performed only if our location is valid.
	bool OptimizePathTopology = true; //!!!to settings!
	if (pActor->NavState == AINav_Following && pActor->IsNavLocationValid() && OptimizePathTopology)
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

	//!!!navmesh height and actual height may differ, so need to compensate or convert actor->navmesh or <- height!
	//???or test with IsAtPoint() with OffMeshRadius?
	if (OffMeshRef && (OffMeshPoint - pActor->Position).SqLength() <= OffMeshRadiusSq)
	{
		// We reached an enter to the offmesh connection that is the next edge of our path (see GetPathEdges())
		//???check validity of the offmesh poly ref? what if we're at the invalid pos now?
		dtPolyRef Refs[2];
		float Dummy[3];
		if (Corridor.moveOverOffmeshConnection(OffMeshRef, Refs, Dummy, OffMeshPoint.v, pNavQuery))
		{
			n_assert(OffMeshRef == Refs[1]);
			TraversingOffMesh = true;
			//???what with distance to nav dest? what if becomes too close to target traversing offmesh?
		}
		else Core::Error("CNavSystem::UpdatePosition() -> Entering invalid offmesh connection");
	}
	else
	{
		if (pActor->IsNavSystemIdle()) ResetPositionPoly(false);
		else
		{
			Corridor.movePosition(pActor->Position.v, pNavQuery, pNavFilter);
			if (pActor->Position.x == Corridor.getPos()[0] && pActor->Position.z == Corridor.getPos()[2])
			{
				n_assert_dbg(pNavQuery->isValidPolyRef(Corridor.getFirstPoly(), pNavFilter));
				pActor->SetNavLocationValid(true);
				if (pBoundary) pBoundary->reset(); //???only if corridor first poly changed?
			}
			else ResetPositionPoly(false);

			if (!pActor->IsNavSystemIdle() && pActor->IsAtPoint(DestPoint, true))
			{
				Reset(true);
				return;
			}
		}
		pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
	}
}
//---------------------------------------------------------------------

void CNavSystem::ResetPositionPoly(bool ForceResetState)
{
	dtPolyRef Ref = 0;

	if (pNavQuery)
	{
		// Find our current poly
		const float Extents[3] = { 0.f, pActor->Height, 0.f };
		float Nearest[3];
		pNavQuery->findNearestPoly(pActor->Position.v, Extents, pNavFilter, &Ref, Nearest);

		// If we're moving, we try to find the nearest valid poly. Else we set invalid
		// curr poly if our position is invalid, without recovery and nearest-searching.
		if (Ref)
		{
			pActor->SetNavLocationValid(pActor->Position.x == Nearest[0] && pActor->Position.z == Nearest[2]);
			if (!pActor->IsNavLocationValid() && pActor->IsNavSystemIdle()) Ref = 0;
		}
		else
		{
			pActor->SetNavLocationValid(false);
			if (!pActor->IsNavSystemIdle())
			{
				// No poly found, try to find nearest valid poly in a recovery radius. Radius may be changed.
				const float RecoveryRadius = n_min(pActor->Radius * 2.f, 20.f);
				const float RecoveryExtents[3] = { RecoveryRadius, pActor->Height, RecoveryRadius };
				pNavQuery->findNearestPoly(pActor->Position.v, RecoveryExtents, pNavFilter, &Ref, NULL);
			}
		}
	}
	else pActor->SetNavLocationValid(false);

	//!!!???if our poly ref changed, trigger some nav poly controller or smth? if we suddenly entered a poly that is
	//blocked by a controller, we can even receive Invalid state fron it here!
	//can get poly action from controller, if poly has controller, else fallback to area + flags + is offmesh
	//!!!need void* UserData in detour nav polys!
	// Smth like:
	// if (Ctlr) Ctlr->OnLeave(pActor);
	// Ctlr = Level->GetPolyController(Ref);
	// if (Ctlr) Ctlr->OnEnter(pActor);
	//!!!can update smth like pActor->GroundInfo here!

	if (ForceResetState || Corridor.getFirstPoly() != Ref)
	{
		Corridor.reset(Ref, pActor->Position.v);
		if (pBoundary) pBoundary->reset();
		if (!pActor->IsNavSystemIdle())
		{
			if (Ref) pActor->NavState = AINav_DestSet;
			else Reset(pActor->IsAtPoint(DestPoint, true));
		}
	}
	else if (pActor->IsNavSystemIdle())
		Corridor.reset(Ref, pActor->Position.v); // movePosition() alternative for the idle state
}
//---------------------------------------------------------------------

void CNavSystem::ResetDestinationPoly()
{
	if (!pNavQuery)
	{
		DestRef = 0;
		return;
	}

	const float Extents[3] = { 0.f, pActor->Height, 0.f };
	float Nearest[3];
	pNavQuery->findNearestPoly(DestPoint.v, Extents, pNavFilter, &DestRef, Nearest);
	if (!DestRef || DestPoint.x != Nearest[0] || DestPoint.z != Nearest[2])
	{
		// Exact destination is unreachable, accept nearest or fail
		if (DestRef && pActor->DoesAcceptNearestValidDestination())
			dtVcopy(DestPoint.v, Nearest);
		else 
		{
			DestRef = 0;
			Reset(false);
			return;
		}
	}

	if (pActor->IsAtPoint(DestPoint, true)) Reset(true);
	else
	{
		pActor->NavState = AINav_DestSet;
		pActor->DistanceToNavDest = dtVdist2D(pActor->Position.v, DestPoint.v);
	}
}
//---------------------------------------------------------------------

// We submit path edges to the external actions, like steering, climbing, jumping, swimming, door traversal etc.
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
	//!!!return proper recovery action!
	if (Ref == 0) return EdgeTypeToAction[0];

	const dtMeshTile* pTile;
	const dtPoly* pPoly;
	pNavMesh->getTileAndPolyByRefUnsafe(Ref, &pTile, &pPoly);
	uchar PolyArea = pPoly->getArea();
	ushort PolyFlags = pPoly->flags; //???need? area crossings - all crossings difference!

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

bool CNavSystem::GetPathEdges(CPathEdge* pOutPath, DWORD MaxCount, DWORD& Count)
{
	if (!pOutPath || !MaxCount) FAIL;
	if (!pNavQuery || (pActor->NavState != AINav_Planning && pActor->NavState != AINav_Following)) FAIL;

	const dtNavMesh* pNavMesh = pNavQuery->getAttachedNavMesh();

	//!!!when add an edge, determine navigation controller for the poly
	//if action not changed, but controller has changed, finalize edge and start new one
	//!!!if poly has controller, can determine action from a controller

	CPathEdge* pEdge = NULL;
	Count = 0;
	if (TraversingOffMesh)
	{
		// We are at the offmesh connection, so add it as the first edge
		pEdge = pOutPath;
		pEdge->Action = GetPolyAction(pNavMesh, OffMeshRef);
		pEdge->Dest = OffMeshPoint;
		pEdge->IsLast = false; //???can offmesh connection be the last edge?
		++Count;
	}
	else if (!pActor->IsNavLocationValid() && !pActor->IsAtPoint(Corridor.getPos(), false))
	{
		// We're recovering from an invalid state, add an edge from the current actor position to
		// the nearest valid navmesh position, that doesn't break route. It also prevents tunneling.
		pEdge = pOutPath;
		pEdge->Action = GetPolyAction(pNavMesh, 0);
		pEdge->Dest = Corridor.getPos();
		pEdge->IsLast = false; // Recovery is always a result of a movement request
		++Count;
	}

	if (Count == MaxCount) OK;

	//!!!since DT_STRAIGHTPATH_#_CROSSINGS is used, more than 3 corners could be requested!
	//???request/implement iterative pNavQuery->getNextCorner until MaxCount edges are formed?
	//???use DT_STRAIGHTPATH_ALL_CROSSINGS? can action change where area doesn't change? if controller, it can!
	const int MAX_CORNERS = 3;
	float CornerVerts[MAX_CORNERS * 3];
	unsigned char CornerFlags[MAX_CORNERS];
	dtPolyRef CornerPolys[MAX_CORNERS];
	int CornerCount;
	pNavQuery->findStraightPath(Corridor.getPos(), Corridor.getTarget(), Corridor.getPath(), Corridor.getPathCount(),
								CornerVerts, CornerFlags, CornerPolys, &CornerCount, MAX_CORNERS, DT_STRAIGHTPATH_AREA_CROSSINGS);

	// The first one is our position, that can be skipped, or the first valid position with recovery edge already added.
	// Skip corners in the arrival tolerance.
	int FirstIdx;
	float ArrivalTolSq = pActor->ArrivalTolerance * pActor->ArrivalTolerance;
	for (FirstIdx = 1; FirstIdx < CornerCount; ++FirstIdx)
	{
		float* pCrn = CornerVerts + FirstIdx * 3;
		if (dtVdist2DSqr(pActor->Position.v, pCrn) > ArrivalTolSq || n_fabs(pActor->Position.y - pCrn[1]) > pActor->Height) break;
	}

	if (FirstIdx >= CornerCount) OK;

	// Code below relies on these assumptions. Normally this can never happen.
	n_assert_dbg(FirstIdx > 0 && !(CornerFlags[0] & DT_STRAIGHTPATH_OFFMESH_CONNECTION));

	//!!!Optimize not every frame but only when needed to fix path!
	// (adjust call frequenCY, and mb force when path changes). More frequent when avoid obstacles
	//???mb actual polys have changed here and we must re-get edges?
	//!!!now optimization effect is sensible only at the next call!
	const float* pNextCorner = &CornerVerts[dtMin(FirstIdx + 1, CornerCount - 1) * 3];
	bool OptimizePathVis = true; //!!!to settings!
	if (OptimizePathVis)
		Corridor.optimizePathVisibility(pNextCorner, 30.f * pActor->Radius, pNavQuery, pNavFilter);

	// Each corner is a potential action or direction change point. Convert corners to path edges.
	OffMeshRef = 0;
	const dtPolyRef* pCurrPoly = Corridor.getPath();
	const dtPolyRef* pLastPoly = Corridor.getPath() + Corridor.getPathCount();
	for (int i = FirstIdx; (i < CornerCount) && (Count < MaxCount); ++i)
	{
		// Corner poly means "the poly that starts from this corner", so we get
		// a previous poly, because we want to know the poly that ends at this corner.
		CStrID CurrAction = GetPolyAction(pNavMesh, CornerPolys[i - 1]);

		// If action or direction changes at this corner, start new edge
		bool StartNewEdge = (!Count) || (pEdge->Action != CurrAction);
		if (!StartNewEdge && Count)
		{
			// Check if horizontal direction changed (more than by very small angle, to handle float errors)
			const vector3& Base = Count > 1 ? pOutPath[Count - 2].Dest : pActor->Position;
			vector2 CurrDir(pEdge->Dest.x - Base.x, pEdge->Dest.z - Base.z);
			vector2 NewDir(CornerVerts[i * 3] - Base.x, CornerVerts[i * 3 + 2] - Base.z);
			float Dot = NewDir.dot(CurrDir);
			const float CosSmallAngleSq = 0.999999f;
			StartNewEdge = (Dot * Dot < NewDir.SqLength() * CurrDir.SqLength() * CosSmallAngleSq);
		}

		if (StartNewEdge)
		{
			pEdge = pOutPath + Count;
			pEdge->Action = CurrAction;
			pEdge->IsLast = false;
			++Count;
		}

		pEdge->Dest.set(&CornerVerts[i * 3]);
		//???edge destination must have height of NavMesh or of actual geometry? Steering may depend on it.
		//pNavQuery->getPolyHeight(CurrPoly, &CornerVerts[i * 3], &pNode->Dest.y);

		// If offmesh connection start encountered, we record its info and UpdatePosition() will detect
		// if we are at the trigger radius to start traversal. We don't gather edges after this point.
		if (CornerFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			OffMeshRef = CornerPolys[CornerCount - 1];
			OffMeshPoint.set(&CornerVerts[(CornerCount - 1) * 3]);
			OffMeshRadiusSq = pActor->Radius * 1.2f; //!!!DetermineOffMeshConnectionTriggerRadius();? from poly controller?
			OffMeshRadiusSq *= OffMeshRadiusSq;
			CornerCount = i + 1;
		}
	}

	pEdge->IsLast = (pActor->NavState == AINav_Following) && (CornerFlags[CornerCount - 1] & DT_STRAIGHTPATH_END);

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

// Finds a valid position in range of [MinRande, MaxRange] from Center, that is closest to the actor.
// Fails if there is no valid location in the specified zone, or if error occured.
// NB: this function can modify OutPos even if failed
bool CNavSystem::GetNearestValidLocation(const vector3& Center, float MinRange, float MaxRange, vector3& OutPos) const
{
	if (!pNavQuery || MinRange > MaxRange) FAIL;

	const float SqMinRange = MinRange * MinRange;
	const float SqMaxRange = MaxRange * MaxRange;

	//???need fast check that we are inside? see through the code below
	//if actor is not in a valid location, but is in range, it may return wrong result
	//so check may use actor nav. status or can be made externally
	//float SqRange = vector3::SqDistance2D(pActor->Position, Center);
	//if (SqRange >= SqMinRange && SqRange <= SqMaxRange) { OutPos = pActor->Position; OK; }

	dtPolyRef Ref;
	vector3 Pt;
	const float Extents[3] = { MaxRange, 1.f, MaxRange };
	pNavQuery->findNearestPoly(Center.v, Extents, pNavFilter, &Ref, Pt.v);
	if (vector3::SqDistance2D(Center, Pt) > SqMaxRange) FAIL;

	if (SqMaxRange == 0.f)
	{
		OutPos = Pt;
		OK;
	}

	dtPolyRef NearRefs[32];
	int NearCount;
	if (!dtStatusSucceed(pNavQuery->findPolysAroundCircle(Ref, Pt.v, MaxRange, pNavFilter, NearRefs, NULL, NULL, &NearCount, 32))) FAIL;

	// Exclude polys laying entirely in MinRange. Since polys are convex, there is no chance
	// that some poly has all corners inside MinRange, but some part of area outside.
	if (SqMinRange > 0.f)
	{
		const dtNavMesh* pNavMesh = pNavQuery->getAttachedNavMesh();
		for (int i = 0; i < NearCount; )
		{
			dtPolyRef NearRef = NearRefs[i];
			const dtMeshTile* pTile;
			const dtPoly* pPoly;
			pNavMesh->getTileAndPolyByRefUnsafe(NearRef, &pTile, &pPoly);

			int j;
			for (j = 0; j < pPoly->vertCount; ++j)
				if (dtVdist2DSqr(pTile->verts + pPoly->verts[j] * 3, Center.v) > SqMinRange) break;

			if (j < pPoly->vertCount) ++i;
			else
			{
				--NearCount;
				if (i < NearCount) NearRefs[i] = NearRefs[NearCount];
			}
		}
	}

	if (NearCount < 1) FAIL;

	dtPolyRef NearestPoly = 0;

	// Optimization: if one of polys found is a poly we are on, select it
	Ref = Corridor.getFirstPoly();
	if (Ref)
		for (int i = 0; i < NearCount; ++i)
			if (NearRefs[i] == Ref)
			{
				NearestPoly = Ref;
				dtVcopy(OutPos.v, Corridor.getPos());
				break;
			}

	if (!NearestPoly)
	{
		//!!!Copied from Detour dtNavMeshQuery::findNearestPoly
		//???to utility function GetNearestPoly(Polys, Point)? ask memononen
		float* pPos = pActor->Position.v;
		float MinSqDist = FLT_MAX;
		for (int i = 0; i < NearCount; ++i)
		{
			dtPolyRef NearRef = NearRefs[i];
			float ClosestPtPoly[3];
			pNavQuery->closestPointOnPoly(NearRef, pPos, ClosestPtPoly);
			float SqDist = dtVdistSqr(pPos, ClosestPtPoly); //???dtVdist2DSqr
			if (SqDist < MinSqDist)
			{
				MinSqDist = SqDist;
				NearestPoly = NearRef;
				dtVcopy(OutPos.v, ClosestPtPoly);
			}
		}
	}

	n_assert_dbg(NearestPoly);

	// If OutPos is in [MinRange, MaxRange], return it, else select closest range

	float SqRange = vector3::SqDistance2D(OutPos, Center);
	if (SqRange > SqMaxRange) SqRange = SqMaxRange;
	else if (SqRange < SqMinRange) SqRange = SqMinRange;
	else OK;

	// Use segment-circle intersection to project OutPos onto the closest range.
	// Segment is defined by OutPos, which is an actor's position projected to the nearest poly,
	// and by ProjCenter, which is a Center projected to the nearest poly, so the segment is the
	// shortest path from an actor to the Center along the nearest poly surface.

	vector3 ProjCenter;
	pNavQuery->closestPointOnPoly(NearestPoly, Center.v, ProjCenter.v);

	vector3 SegDir = OutPos - ProjCenter;
	vector3 RelProjCenter = ProjCenter - Center;
	float A = SegDir.SqLength2D();
	float B = 2.f * RelProjCenter.Dot2D(SegDir);
	float C = RelProjCenter.SqLength2D() - SqRange;
	float t1, t2;
	DWORD RootCount = Math::SolveQuadraticEquation(A, B, C, &t1, &t2);
	n_assert2(RootCount, "No solution found for the closest point though theoretically there must be one");
	float t = (RootCount == 1 || t1 > t2) ? t1 : t2; // Greater t is closer to actor, which is desired
	n_assert_dbg(t >= 0.f && t <= 1.f);
	OutPos = ProjCenter + SegDir * t;

	OK;
}
//---------------------------------------------------------------------

void CNavSystem::RenderDebug()
{
	if (!pNavQuery) return;

	LPCSTR pNavStr = NULL;
	if (pActor->NavState == AINav_Done) pNavStr = "Done";
	else if (pActor->NavState == AINav_Failed) pNavStr = "Failed";
	else if (pActor->NavState == AINav_DestSet) pNavStr = "DestSet";
	else if (pActor->NavState == AINav_Planning) pNavStr = "Planning";
	else if (pActor->NavState == AINav_Following) pNavStr = "Following";

	CString Text;
	Text.Format(
		"Nav state: %s\n"
		"Nav location is %s\n"
		"Curr poly: %d\n"
		"Dest poly: %d\n"
		"Destination: %.4f, %.4f, %.4f\n",
		pNavStr,
		pActor->IsNavLocationValid() ? "valid" : "invalid",
		Corridor.getFirstPoly(),
		DestRef,
		DestPoint.x, DestPoint.y, DestPoint.z);
	DebugDraw->DrawText(Text.CStr(), 0.65f, 0.1f);

	// Path polys, path lines with corners as points
	if (pActor->NavState == AINav_Planning || pActor->NavState == AINav_Following)
	{
		static const vector4 ColorPathLine(1.f, 0.75f, 0.5f, 1.f);
		static const vector4 ColorPathCorner(1.f, 0.9f, 0.f, 1.f);
		static const unsigned int ColorPoly = duRGBA(255, 196, 0, 64);

		CNavMeshDebugDraw DD;
		for (int i = 0; i < Corridor.getPathCount(); ++i)
			duDebugDrawNavMeshPoly(&DD, *pNavQuery->getAttachedNavMesh(), Corridor.getPath()[i], ColorPoly);

		const int MAX_CORNERS = 32;
		float CornerVerts[MAX_CORNERS * 3];
		unsigned char CornerFlags[MAX_CORNERS];
		dtPolyRef CornerPolys[MAX_CORNERS];
		int CornerCount;
		pNavQuery->findStraightPath(Corridor.getPos(), Corridor.getTarget(), Corridor.getPath(), Corridor.getPathCount(),
									CornerVerts, CornerFlags, CornerPolys, &CornerCount, MAX_CORNERS, DT_STRAIGHTPATH_AREA_CROSSINGS);
		vector3 From = pActor->Position;
		for (int i = 0; i < CornerCount; ++i)
		{
			vector3 To = CornerVerts + (i * 3);
			DebugDraw->DrawLine(From, To, ColorPathLine);
			DebugDraw->DrawPoint(To, 5, ColorPathCorner);
			From = To;
		}
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