#pragma once
#include <StdDEM.h>
#include <DetourNavMesh.h>

// Queue of pathfinding requests.
// Based on dtPathQueue, but allows each request to have its own dtNavMeshQuery ptr.

//???write template queue?

class dtNavMeshQuery;
class dtQueryFilter;

namespace DEM::AI
{

class CPathRequestQueue
{
protected:

	struct CPathQuery
	{
		U16						RequestID;
		float					StartPos[3];
		float					EndPos[3];
		dtPolyRef				StartRef;
		dtPolyRef				EndRef;
		dtPolyRef*				pPath = nullptr;
		int						PathSize;
		dtStatus				Status;
		int						KeepAlive;
		dtNavMeshQuery*			pNavQuery = nullptr;
		const dtQueryFilter*	pFilter = nullptr;	///< TODO: This is potentially dangerous!
	};

	static const int MAX_QUEUE = 8;

	CPathQuery	Queue[MAX_QUEUE];
	U16			NextRequestID = 1;
	int			MaxPathSize = 0;
	int			QueueHead = 0;
	
	void		Purge();

public:

	~CPathRequestQueue() { Purge(); }

	bool		Init(int MaxPath);
	void		Update(int MaxIters);
	U16			Request(dtPolyRef RStart, dtPolyRef REnd, const float* pStart, const float* pEnd, dtNavMeshQuery* pNavQuery, const dtQueryFilter* pFilter);
	void		CancelRequest(U16 RequestID);
	dtStatus	GetRequestStatus(U16 RequestID) const;
	dtStatus	GetPathResult(U16 RequestID, dtPolyRef* pOutPath, int& OutSize, int MaxPath);
	int			GetPathSize(U16 RequestID);
};

}
