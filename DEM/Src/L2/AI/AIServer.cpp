#include "AIServer.h"

#include <AI/Planning/ActionTpl.h>
#include <AI/Behaviour/Action.h>
#include <Data/Params.h>
#include <Events/EventServer.h>
#include <Core/Factory.h>
#include <DetourNavMeshQuery.h>
#include <DetourObstacleAvoidance.h>

namespace AI
{
__ImplementClassNoFactory(AI::CAIServer, Core::CObject);
__ImplementSingleton(AI::CAIServer);

CAIServer::CAIServer()
{
	__ConstructSingleton;

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

	dtQueryFilter* pNavFilter = n_new(dtQueryFilter);
	pNavFilter->setExcludeFlags(NAV_FLAG_LOCKED);
	NavQueryFilters.Add(CStrID::Empty, pNavFilter);

	pDebugFilter = n_new(dtQueryFilter);

	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		n_assert(PathQueues[i].Init(MAX_NAV_PATH)); //???reinit on each level loading?
}
//---------------------------------------------------------------------

CAIServer::~CAIServer()
{
	n_delete(pDebugFilter);

	for (UPTR i = 0; i < NavQueryFilters.GetCount(); ++i)
		n_delete(NavQueryFilters.ValueAt(i));

	for (UPTR i = 0; i < ObstacleAvoidanceParams.GetCount(); ++i)
		n_delete(ObstacleAvoidanceParams.ValueAt(i));

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CAIServer::Trigger()
{
	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		PathQueues[i].Update(100);
}
//---------------------------------------------------------------------

void CAIServer::AddSmartAction(CStrID ID, const Data::CParams& Desc)
{
	if (!SOActTpls.Contains(ID))
	{
		CSmartAction& SA = SOActTpls.Add(ID);
		SA.Init(ID, Desc);
	}
}
//---------------------------------------------------------------------

const CSmartAction* CAIServer::GetSmartAction(CStrID ID) const
{
	int Idx = SOActTpls.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &SOActTpls.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

PAction CAIServer::CreatePlanFromDesc(Data::PParams Desc)
{
	if (Desc.IsNullPtr()) return NULL;

	CString StrClass("AI::CAction");
	StrClass += Desc->Get<CString>(CStrID("Class"));
	PAction Plan = (CAction*)Factory->Create(StrClass);
	Plan->Init(*Desc);
	return Plan;
}
//---------------------------------------------------------------------

}