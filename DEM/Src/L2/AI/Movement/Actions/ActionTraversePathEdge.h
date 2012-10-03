#pragma once
#ifndef __DEM_L2_AI_ACTION_TRAVERSE_PATH_EDGE_H__
#define __DEM_L2_AI_ACTION_TRAVERSE_PATH_EDGE_H__

#include <AI/Behaviour/Action.h>

// Goto action that sets destination from static or dynamic target object.

namespace AI
{
struct CPathEdge;

class CActionTraversePathEdge: public CAction
{
	DeclareRTTI;

protected:

public:

	virtual void UpdatePathEdge(CActor* pActor, const CPathEdge* pEdge, const CPathEdge* pNextEdge) = 0;
};

typedef Ptr<CActionTraversePathEdge> PActionTraversePathEdge;

}

#endif