#pragma once
#ifndef __DEM_L2_AI_ACTION_STEER_TO_POS_H__
#define __DEM_L2_AI_ACTION_STEER_TO_POS_H__

#include "ActionTraversePathEdge.h"

// Path traversal action that simply steers actor to the destination point of a path edge.
// Steering and movement parameters are passed to the MotorSystem.

namespace AI
{

class CActionSteerToPosition: public CActionTraversePathEdge
{
	__DeclareClass(CActionSteerToPosition);

private:

public:

	virtual void		UpdatePathEdge(CActor* pActor, const CPathEdge* pEdge, const CPathEdge* pNextEdge);

	//virtual bool		Activate(CActor* pActor);
	virtual DWORD	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
};

}

#endif