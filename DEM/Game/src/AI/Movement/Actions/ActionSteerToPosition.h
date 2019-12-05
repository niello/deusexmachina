#pragma once
#include "ActionTraversePathEdge.h"

// Path traversal action that simply steers actor to the destination point of a path edge.
// Steering and movement parameters are passed to the MotorSystem.

namespace AI
{

class CActionSteerToPosition: public CActionTraversePathEdge
{
	FACTORY_CLASS_DECL;

private:

public:

	virtual void	UpdatePathEdge(CActor* pActor, const CPathEdge* pEdge, const CPathEdge* pNextEdge);

	//virtual bool		Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);
};

}
