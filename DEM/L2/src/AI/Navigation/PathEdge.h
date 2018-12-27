#pragma once
#ifndef __DEM_L2_AI_PATH_EDGE_H__
#define __DEM_L2_AI_PATH_EDGE_H__

#include <Data/StringID.h>
#include <Math/Vector3.h>

// Path node is a waypoint/edge representation, produced as part of a path by path planner.
// Path is just an array of path nodes.

namespace AI
{

struct CPathEdge
{
	vector3 Dest;			// Far point of the edge (destination)
	CStrID	Action;			// Action required from the actor to traverse the edge
	//float	ReachRadius;	// Distance from destination when we assume it is reached
	bool	IsLast;			// Is this edge destination a whole path destination
};

}

#endif