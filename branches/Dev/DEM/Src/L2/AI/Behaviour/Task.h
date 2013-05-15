#pragma once
#ifndef __DEM_L2_AI_TASK_H__
#define __DEM_L2_AI_TASK_H__

#include <AI/ActorFwd.h>
#include <Data/Params.h>
#include <Core/RefCounted.h>

// Task is a behaviour (plan) source opposite to goal. Task provides actor with hardcoded or scripted plan,
// whereas goal only sets a desired world state and allows planner to build a plan.
// May be later task and goal will be inherited from the common base or task will become a special case of goal.

namespace AI
{
typedef Ptr<class CAction> PAction;

class CTask: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	float	Relevance;
	bool	Done;

public:

	CTask(): Relevance(0.f), Done(false) {}

	virtual void	Init(Data::PParams Desc) {}
	virtual bool	IsAvailableTo(const CActor* pActor) { OK; }
	virtual void	EvalRelevance(const CActor* pActor) { Relevance = 0.f; }
	virtual PAction	BuildPlan() = 0;
	virtual bool	OnPlanSet(CActor* pActor) { OK; }
	virtual void	OnPlanDone(CActor* pActor, EExecStatus BhvResult) { Done = true; }
	virtual void	Abort(CActor* pActor) { }

	bool			IsSatisfied() const { return Done; }
	float			GetRelevance() const { return Relevance; }
};

typedef Ptr<CTask> PTask;

}

#endif