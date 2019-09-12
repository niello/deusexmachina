#include "PathRequestQueue.h"

#include <System/System.h>
#include <DetourCommon.h>
#include <DetourNavMeshQuery.h>
#include <memory.h> // memcpy

namespace AI
{

void CPathRequestQueue::Purge()
{
	for (int i = 0; i < MAX_QUEUE; ++i)
	{
		dtFree(Queue[i].pPath);
		Queue[i].pPath = nullptr;
	}
}
//---------------------------------------------------------------------

bool CPathRequestQueue::Init(int MaxPath)
{
	Purge();
	
	MaxPathSize = MaxPath;
	for (int i = 0; i < MAX_QUEUE; ++i)
		Queue[i].RequestID = DT_PATHQ_INVALID;
	
	QueueHead = 0;
	OK;
}
//---------------------------------------------------------------------

void CPathRequestQueue::Update(int MaxIters)
{
	static const int MAX_KEEP_ALIVE = 2; // In update ticks

	int IterCount = MaxIters;
	
	for (int i = 0; i < MAX_QUEUE; ++i)
	{
		CPathQuery& Query = Queue[QueueHead % MAX_QUEUE];
		
		if (Query.RequestID == DT_PATHQ_INVALID)
		{
			++QueueHead;
			continue;
		}
		
		if (dtStatusSucceed(Query.Status) || dtStatusFailed(Query.Status))
		{
			++Query.KeepAlive;
			if (Query.KeepAlive > MAX_KEEP_ALIVE)
			{
				Query.RequestID = DT_PATHQ_INVALID;
				Query.Status = 0;
			}
			
			++QueueHead;
			continue;
		}
		
		if (Query.Status == 0)
			Query.Status = Query.pNavQuery->initSlicedFindPath(Query.StartRef, Query.EndRef, Query.StartPos, Query.EndPos, Query.pFilter);

		if (dtStatusInProgress(Query.Status))
		{
			int Iters = 0;
			Query.Status = Query.pNavQuery->updateSlicedFindPath(IterCount, &Iters);
			IterCount -= Iters;
		}

		if (dtStatusSucceed(Query.Status))
			Query.Status = Query.pNavQuery->finalizeSlicedFindPath(Query.pPath, &Query.PathSize, MaxPathSize);

		if (IterCount <= 0) break;

		++QueueHead;
	}
}
//---------------------------------------------------------------------

UPTR CPathRequestQueue::Request(dtPolyRef RStart, dtPolyRef REnd, const float* pStart,
								 const float* pEnd, dtNavMeshQuery* pNavQuery, const dtQueryFilter* pFilter)
{
	n_assert(pStart && pEnd && pNavQuery);

	int i;
	for (i = 0; i < MAX_QUEUE; ++i)
		if (Queue[i].RequestID == DT_PATHQ_INVALID) break;

	if (i == MAX_QUEUE) return DT_PATHQ_INVALID;
	
	CPathQuery& Query = Queue[i];
	Query.RequestID = NextRequestID++;
	if (NextRequestID == DT_PATHQ_INVALID) NextRequestID++;
	dtVcopy(Query.StartPos, pStart);
	Query.StartRef = RStart;
	dtVcopy(Query.EndPos, pEnd);
	Query.EndRef = REnd;
	Query.Status = 0;
	Query.PathSize = 0;
	Query.pNavQuery = pNavQuery;
	Query.pFilter = pFilter;
	Query.KeepAlive = 0;

	if (!Queue[i].pPath)
	{
		Queue[i].pPath = (dtPolyRef*)dtAlloc(sizeof(dtPolyRef) * MaxPathSize, DT_ALLOC_PERM);
		n_assert(Queue[i].pPath);
	}

	return Query.RequestID;
}
//---------------------------------------------------------------------

void CPathRequestQueue::CancelRequest(UPTR RequestID)
{
	for (int i = 0; i < MAX_QUEUE; ++i)
		if (Queue[i].RequestID == RequestID)
		{
			CPathQuery& Query = Queue[i];
			Query.RequestID = DT_PATHQ_INVALID;
			Query.Status = 0;
		}
}
//---------------------------------------------------------------------

dtStatus CPathRequestQueue::GetRequestStatus(UPTR RequestID) const
{
	for (int i = 0; i < MAX_QUEUE; ++i)
		if (Queue[i].RequestID == RequestID)
			return Queue[i].Status;
	return DT_FAILURE;
}
//---------------------------------------------------------------------

dtStatus CPathRequestQueue::GetPathResult(UPTR RequestID, dtPolyRef* pOutPath, int& OutSize, int MaxPath)
{
	for (int i = 0; i < MAX_QUEUE; ++i)
		if (Queue[i].RequestID == RequestID)
		{
			CPathQuery& Query = Queue[i];
			Query.RequestID = DT_PATHQ_INVALID;
			Query.Status = 0;
			OutSize = dtMin(Query.PathSize, MaxPath);
			memcpy(pOutPath, Query.pPath, sizeof(dtPolyRef) * OutSize);
			return DT_SUCCESS;
		}
	return DT_FAILURE;
}
//---------------------------------------------------------------------

}