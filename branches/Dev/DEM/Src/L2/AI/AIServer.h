#pragma once
#ifndef __DEM_L2_AI_SERVER_H__
#define __DEM_L2_AI_SERVER_H__

#include <Core/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <AI/Planning/Planner.h>
#include <AI/SmartObj/SmartAction.h>
#include <AI/AILevel.h>

// AI server manages AI levels and high-level systems like planner or memory fact factory

namespace Data
{
	class CParams;
}

class dtQueryFilter;
struct dtObstacleAvoidanceParams;
typedef dtObstacleAvoidanceParams COAParams;

namespace AI
{
#define AISrv AI::CAIServer::Instance()

class CAIServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CAIServer);

private:

	CPlanner							Planner; //???or singleton?
	CDict<CStrID, CSmartAction>	SOActTpls;
	CDict<CStrID, dtQueryFilter*>		NavQueryFilters;
	dtQueryFilter*						pDebugFilter;
	CDict<CStrID, COAParams*>			ObstacleAvoidanceParams;
	CPathRequestQueue					PathQueues[DEM_THREAD_COUNT];

public:

	CAIServer();
	~CAIServer();

	void					Trigger();

	void					AddSmartAction(CStrID ID, const Data::CParams& Desc);
	const CSmartAction*		GetSmartAction(CStrID ID) const;

	void					AddNavQueryFilter(CStrID ID, const Data::CParams& Desc);
	const dtQueryFilter*	GetNavQueryFilter(CStrID ID) const;
	const dtQueryFilter*	GetDefaultNavQueryFilter() const { return GetNavQueryFilter(CStrID::Empty); }
	const dtQueryFilter*	GetDebugNavQueryFilter() const { return pDebugFilter; }

	void					AddObstacleAvoidanceParams(CStrID ID, const Data::CParams& Desc);
	const COAParams*		GetObstacleAvoidanceParams(CStrID ID) const;
	const COAParams*		GetDefaultObstacleAvoidanceParams() const { return GetObstacleAvoidanceParams(CStrID::Empty); }

	CPathRequestQueue*		GetPathQueue(DWORD ThreadID = 0) { n_assert(ThreadID < DEM_THREAD_COUNT); return PathQueues + ThreadID; }

	CPlanner&				GetPlanner() { return Planner; } //???or singleton?
	static PAction			CreatePlanFromDesc(Data::PParams Desc);
};

inline const dtQueryFilter* CAIServer::GetNavQueryFilter(CStrID ID) const
{
	int Idx = NavQueryFilters.FindIndex(ID);
	return Idx != INVALID_INDEX ? NavQueryFilters.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

inline const COAParams* CAIServer::GetObstacleAvoidanceParams(CStrID ID) const
{
	int Idx = ObstacleAvoidanceParams.FindIndex(ID);
	return Idx != INVALID_INDEX ? ObstacleAvoidanceParams.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
