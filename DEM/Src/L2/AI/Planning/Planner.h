#pragma once
#ifndef __DEM_L2_AI_PLANNER_H__
#define __DEM_L2_AI_PLANNER_H__

#include <AI/Planning/WorldState.h>
#include <AI/Planning/ActionTpl.h>
#include <AI/ActorFwd.h>
#include <System/Allocators/PoolAllocator.h>

// Planner receives goal as input and builds plan (action sequence) that will satisfy
// the goal when is completed.
// Planner is a singleton, this is achieved by having an instance in the singleton AISrv.

extern const CString StrActTplPrefix;

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace AI
{
class CGoal;

class CPlanner
{
private:

	struct CNode
	{
		CActionTpl*	pAction;
		CWorldState	WSCurr;
		CWorldState	WSGoal;
		CWorldState	WSPreconditions;
		bool		HasPreconditions; //???track num of props set in CWorldState?
		int			Goal;
		int			Fitness;	// Goal + Heuristic
		CNode*		pParent;

		CNode(): pAction(NULL), pParent(NULL), Fitness(MAX_SDWORD) {}
	};

	CPoolAllocator<CNode, 32>	NodePool;
	CArray<PActionTpl>			ActionTpls;
	CArray<CActionTpl*>			EffectToActions[WSP_Count];
	UPTR						NewActIdx;

	static int CmpPlannerNodes(const void* First, const void* Second);

	void	MergeWorldStates(CWorldState& WSCurr, const CWorldState& WSGoal, const CWorldState& WSActor);
	bool	IsPlanValid(CActor* pActor, CNode* pNode, const CWorldState& WSActor);
	void	FillNeighbors(CActor* pActor, const CNode& Node, CArray<CNode*>& OutNeighbors);

public:

	CPlanner(): NewActIdx(0) {}

	void				RegisterActionTpl(const char* Name, Data::PParams Params = NULL);
	void				EndActionTpls();
	const CActionTpl*	FindActionTpl(const char* Name) const;

	PAction				BuildPlan(CActor* pActor, CGoal* pGoal);
};
//---------------------------------------------------------------------

inline const CActionTpl* CPlanner::FindActionTpl(const char* Name) const
{
	CString ClassName = StrActTplPrefix + Name;
	for (CArray<PActionTpl>::CIterator ppTpl = ActionTpls.Begin(); ppTpl != ActionTpls.End(); ++ppTpl)
		if ((*ppTpl)->IsInstanceOf(ClassName))
			return (*ppTpl).GetUnsafe();
	return NULL;
}
//---------------------------------------------------------------------

}

#endif