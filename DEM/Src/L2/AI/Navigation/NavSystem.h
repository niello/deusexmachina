#pragma once
#ifndef __DEM_L2_AI_NAV_SYSTEM_H__
#define __DEM_L2_AI_NAV_SYSTEM_H__

#include <StdDEM.h>
#include <Core/Ptr.h>
#include <AI/Navigation/PathEdge.h>
#include <AI/ActorFwd.h>
#include <Data/Dictionary.h>
#include <Data/String.h>
#include <DetourPathCorridor.h>

// Navigation system is responsible for path planning, destination tracking, high-level spatial status tracking.
// Low-level movement, facing etc are performed in the MotorSystem.

//!!!DT_MAX_AREAS as invalid for GetAreaUnderTheFeet!

namespace Data
{
	typedef Ptr<class CParams> PParams;
};

class dtLocalBoundary;
class dtObstacleAvoidanceQuery;

namespace AI
{
#define NAV_IDLE 0x80

enum ENavState
{
	AINav_Failed = NAV_IDLE,		// Actor failed to reach the destination, NavSystem is idle
	AINav_Done = (NAV_IDLE | 1),	// Actor is at the destination, NavSystem is idle
	AINav_DestSet = 2,				// Destination is set and path is required, try to find quick path with sync query
	AINav_Planning = 3,				// Fast sync pathfinding was not succeed, performing full async pathfinding
	AINav_Following = 4				// Actor has valid path and follows it
};

class CPathRequestQueue;

class CNavSystem
{
protected:

	CActor*					pActor;

	dtNavMeshQuery*			pNavQuery;
	const dtQueryFilter*	pNavFilter;
	dtPathCorridor			Corridor;
	dtLocalBoundary*		pBoundary;

	vector3					DestPoint;
	dtPolyRef				DestRef;

	dtPolyRef				OffMeshRef;
	vector3					OffMeshPoint;
	float					OffMeshRadiusSq;
	bool					TraversingOffMesh;

	float					ReplanTime;
	float					TopologyOptTime;
	CPathRequestQueue*		pProcessingQueue;
	DWORD					PathRequestID;

	//???personal or template?
	CDict<int, CStrID>		EdgeTypeToAction;

	//!!!Path info cache

	dtPolyRef	GetNearestPoly(dtPolyRef* pPolys, int PolyCount, vector3& OutPos) const;
	CStrID		GetPolyAction(const dtNavMesh* pNavMesh, dtPolyRef Ref);
	void		ResetPositionPoly(bool ForceResetState);
	void		ResetDestinationPoly();
	void		ResetPathRequest();

public:

	CNavSystem(CActor* Actor);

	void			Init(const Data::CParams* Params);
	void			Term();
	void			Update(float FrameTime);
	void			Reset(bool Success);
	void			SetupState();
	void			RenderDebug();

	void			UpdatePosition();
	void			EndEdgeTraversal();
	bool			GetPathEdges(CPathEdge* pOutPath, DWORD MaxCount, DWORD& Count);
	void			GetObstacles(float Range, dtObstacleAvoidanceQuery& Query);

	bool			IsPolyValid(dtPolyRef Poly) const { return pNavQuery && pNavQuery->isValidPolyRef(Poly, pNavFilter); }
	DWORD			GetValidPolys(const vector3& Center, float MinRange, float MaxRange, CArray<dtPolyRef>& Polys) const;

	bool			IsLocationValid(const vector3& Point) const;
	bool			GetNearestValidLocation(const vector3& Center, float MinRange, float MaxRange, vector3& OutPos) const;
	bool			GetNearestValidLocation(CStrID NavRegionID, float Range, vector3& OutPos) const;
	bool			GetNearestValidLocation(dtPolyRef* pPolys, int PolyCount, float Range, vector3& OutPos) const;
	bool			GetNearestValidLocation(dtPolyRef* pPolys, int PolyCount, const vector3& Center, float MinRange, float MaxRange, vector3& OutPos) const;
	//bool			GetRandomValidLocation(float ActorRadius, const vector3& Center, float Range, vector3& OutPos) const;

	bool			IsTraversingOffMesh() const { return TraversingOffMesh; }
	void			SetDestPoint(const vector3& Dest);
	const vector3&	GetDestPoint() const { return DestPoint; }
};

}

#endif