#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_H__
#define __DEM_L2_AI_ACTION_GOTO_H__

#include <AI/Behaviour/ActionSequence.h>
#include <AI/Navigation/PathEdge.h>
#include "ActionTraversePathEdge.h"
#include <Data/StringID.h>

// Abstract Goto action. Plans path to the current destination and traverses path edges.
// If the path is built time-sliced (asynchronously), this action can temporarily enable
// steering towards the destination. Now this behaviour is not implemented.

namespace AI
{

class CActionGoto: public CAction
{
	__DeclareClass(CActionGoto);

protected:

	nArray<CPathEdge>		Path;
	CStrID					SubActionID;
	PActionTraversePathEdge SubAction;

	EExecStatus AdvancePath(CActor* pActor);

public:

	//virtual bool		Activate(CActor* pActor);
	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
	virtual bool		IsValid(CActor* pActor) const;
};

typedef Ptr<CActionGoto> PActionGoto;

}

#endif