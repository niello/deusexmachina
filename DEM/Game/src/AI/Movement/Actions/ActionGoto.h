#pragma once
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
	FACTORY_CLASS_DECL;

protected:

	CPathEdge				Path[2];
	CStrID					SubActionID;
	PActionTraversePathEdge SubAction;

	UPTR AdvancePath(CActor* pActor);

public:

	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);
	virtual bool	IsValid(CActor* pActor) const;
};

typedef Ptr<CActionGoto> PActionGoto;

}
