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
#define NAV_IDLE	0x80
#define NAV_INVALID	0x40

enum ENavState
{
	AINav_Done = NAV_IDLE,							// Actor is at the destination, NavSystem is idle
	AINav_Failed = (NAV_IDLE | 1),					// Actor failed to reach the destination, NavSystem is idle
	AINav_DestSet = 2,								// Destination is set and path is required, trying to find path fast and sync
	AINav_Planning = 3,								// Fast sync pathfinding was not succeed, performing full async pathfinding
	AINav_Following = 4,							// Actor has valid path and follows it
	AINav_Invalid = (NAV_INVALID | AINav_DestSet),	// Current actor's position is invalid for navigation, but destination is set
	AINav_IdleInvalid = (NAV_IDLE | NAV_INVALID)	// Current actor's position is invalid for navigation, NavSystem is idle
};

class CPathRequestQueue;

class CNavSystem
{
protected:

	CActor*						pActor;

	dtNavMeshQuery*				pNavQuery;
	const dtQueryFilter*		pNavFilter;
	dtPathCorridor				Corridor;
	dtLocalBoundary*			pBoundary;

	vector3						DestPoint;
	dtPolyRef					DestRef;

	dtPolyRef					OffMeshRef;
	vector3						OffMeshPoint;
	float						OffMeshRadiusSq;
	bool						TraversingOffMesh;

	float						ReplanTime;
	float						TopologyOptTime;
	CPathRequestQueue*			pProcessingQueue;
	DWORD						PathRequestID;

	//???personal or template?
	CDict<int, CStrID>			EdgeTypeToAction;

	//!!!Path info cache

	CStrID	GetPolyAction(const dtNavMesh* pNavMesh, dtPolyRef Ref);
	void	UpdateDestination();
	void	ResetPathRequest();

public:

	CNavSystem(CActor* Actor);

	void			Init(const Data::CParams* Params);
	void			Term();
	void			Update(float FrameTime);
	void			Reset();
	void			SetupState();
	void			RenderDebug();

	void			UpdatePosition();
	void			EndEdgeTraversal();
	bool			GetPathEdges(CArray<CPathEdge>& OutPath, int MaxSize = MAX_SDWORD);
	void			GetObstacles(float Range, dtObstacleAvoidanceQuery& Query);
	//bool			GetRandomValidLocation(float Range, vector3& Location);

	bool			IsTraversingOffMesh() const { return TraversingOffMesh; }
	void			SetDestPoint(const vector3& Dest);
	const vector3&	GetDestPoint() const { return DestPoint; }
};

}

#endif