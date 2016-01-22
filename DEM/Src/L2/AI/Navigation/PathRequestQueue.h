#pragma once
#ifndef __DEM_L2_AI_PATH_REQUEST_QUEUE_H__
#define __DEM_L2_AI_PATH_REQUEST_QUEUE_H__

#include <StdDEM.h>
#include <DetourNavMesh.h>

// Queue of pathfinding requests.
// Based on dtPathQueue, but allows each request to have its own dtNavMeshQuery ptr.

//???write template queue?

class dtNavMeshQuery;
class dtQueryFilter;

static const unsigned int DT_PATHQ_INVALID = 0;

namespace AI
{

class CPathRequestQueue
{
protected:

	struct CPathQuery
	{
		UPTR					RequestID;
		float					StartPos[3];
		float					EndPos[3];
		dtPolyRef				StartRef;
		dtPolyRef				EndRef;
		dtPolyRef*				pPath;
		int						PathSize;
		dtStatus				Status;
		int						KeepAlive;
		dtNavMeshQuery*			pNavQuery;
		const dtQueryFilter*	pFilter;	///< TODO: This is potentially dangerous!
	};

	static const int MAX_QUEUE = 8;

	CPathQuery	Queue[MAX_QUEUE];
	UPTR		NextRequestID;
	int			MaxPathSize;
	int			QueueHead;
	
	void		Purge();

public:

	CPathRequestQueue();
	~CPathRequestQueue() { Purge(); }

	bool		Init(int MaxPath);
	void		Update(int MaxIters);
	UPTR		Request(dtPolyRef RStart, dtPolyRef REnd, const float* pStart, const float* pEnd, dtNavMeshQuery* pNavQuery, const dtQueryFilter* pFilter);
	void		CancelRequest(UPTR RequestID);
	dtStatus	GetRequestStatus(UPTR RequestID) const;
	dtStatus	GetPathResult(UPTR RequestID, dtPolyRef* pOutPath, int& OutSize, int MaxPath);
};

inline CPathRequestQueue::CPathRequestQueue(): NextRequestID(1)
{
	for (int i = 0; i < MAX_QUEUE; ++i)
		Queue[i].pPath = NULL;
}
//---------------------------------------------------------------------

}

#endif
