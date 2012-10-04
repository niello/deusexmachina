#include "AIServer.h"

#include <AI/Planning/ActionTpl.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <Data/Params.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>
#include <DetourNavMeshQuery.h>
#include <DetourObstacleAvoidance.h>
#include <DetourDebugDraw.h>

namespace AI
{
ImplementRTTI(AI::CAIServer, Core::CRefCounted);
ImplementFactory(AI::CAIServer);
__ImplementSingleton(AI::CAIServer);

CAIServer::CAIServer()
{
	__ConstructSingleton;
	DataSrv->SetAssign("actors", "game:ai/actors"); //!!!unwind!
	DataSrv->SetAssign("aihints", "game:ai/hints");
	DataSrv->SetAssign("smarts", "game:ai/smarts");

	dtObstacleAvoidanceParams* pOAParams = n_new(dtObstacleAvoidanceParams);
	pOAParams->velBias = 0.4f;
	pOAParams->weightDesVel = 2.0f;
	pOAParams->weightCurVel = 0.75f;
	pOAParams->weightSide = 0.75f;
	pOAParams->weightToi = 2.5f;
	pOAParams->horizTime = 2.5f;
	pOAParams->gridSize = 33;
	pOAParams->adaptiveDivs = 7;
	pOAParams->adaptiveRings = 2;
	pOAParams->adaptiveDepth = 5;
	ObstacleAvoidanceParams.Add(CStrID::Empty, pOAParams);

	NavQueryFilters.Add(CStrID::Empty, n_new(dtQueryFilter));

	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		n_assert(PathQueues[i].Init(MAX_NAV_PATH)); //???reinit on each level loading?
}
//---------------------------------------------------------------------

CAIServer::~CAIServer()
{
	for (int i = 0; i < NavQueryFilters.Size(); ++i)
		n_delete(NavQueryFilters.ValueAtIndex(i));

	for (int i = 0; i < ObstacleAvoidanceParams.Size(); ++i)
		n_delete(ObstacleAvoidanceParams.ValueAtIndex(i));

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CAIServer::SetupLevel(const bbox3& Bounds)
{
	//!!!kill prev level if was!

	CurrLevel = n_new(CAILevel);

	//!!!calc depth based on level size or read from settings!
	return CurrLevel->Init(Bounds, 5);
}
//---------------------------------------------------------------------

void CAIServer::RenderDebug()
{
	// Render the first NavMesh (later render navmesh used by the current actor)
	if (CurrLevel.isvalid())
	{
		dtNavMeshQuery* pNavQuery = CurrLevel->GetSyncNavQuery(0.f);
		if (pNavQuery)
		{
			CNavMeshDebugDraw DD;

			nGfxServer2::Instance()->BeginShapes();
			duDebugDrawNavMesh(&DD, *pNavQuery->getAttachedNavMesh(), DU_DRAWNAVMESH_OFFMESHCONS);
			//duDebugDrawNavMeshBVTree(&DD, *pNavQuery->getAttachedNavMesh());
			nGfxServer2::Instance()->EndShapes();
		}
	}
}
//---------------------------------------------------------------------

void CAIServer::AddSmartObjActionTpl(CStrID ID, const CParams& Desc)
{
	if (!SOActTpls.Contains(ID)) SOActTpls.Add(ID, CSmartObjActionTpl(Desc));
}
//---------------------------------------------------------------------

const CSmartObjActionTpl* CAIServer::GetSmartObjActionTpl(CStrID ID) const
{
	int Idx = SOActTpls.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &SOActTpls.ValueAtIndex(Idx) : NULL;
}
//---------------------------------------------------------------------

} //namespace AI