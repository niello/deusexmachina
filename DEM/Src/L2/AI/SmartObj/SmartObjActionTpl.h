#pragma once
#ifndef __DEM_L2_IAO_USE_ACTION_TPL_H__
#define __DEM_L2_IAO_USE_ACTION_TPL_H__

#include <Data/Flags.h>
#include <Core/RefCounted.h>

// Template record for smart object action, contains shared properties
// to reduce memory footprint and setup efforts

namespace Data
{
	class CParams;
}

namespace AI
{

struct CSmartObjActionTpl
{
	enum
	{
		FACE_OBJECT				= 0x01,	// If set, actor will face host object before starting the action
		END_ON_DONE				= 0x02,	// If set, end action when Duration passed
		RESET_ON_ABORT			= 0x04,	// If set, clear progress of unfinished action
		LOOP_ANIM				= 0x08,	// If set, action is done after Duration passed, anim looped, else done on anim done
		ACTOR_RADIUS_MATTERS	= 0x10	// If set, distance is adjusted by the actor radius
		// - disable after use
	};

	Data::CFlags	Flags;

	// Destination
	float			MinDistance;		// Can make union with navmesh region
	float			MaxDistance;		// < 0 = no distance requirements
	vector3			DestOffset;			// Offset from SO position in local coords

	// Facing
	// - local face direction (angle from forward direction around Y axis)

	// Animation and timing
	float			Duration;			// Only for LOOP_ANIM. Can split to min & max to randomize
	// - anim props or name or id
	// - fidget period min, max (actor does some short non-looped busy animation)

	// Usage restrictions
	int				MaxUserCount;		// How much users at a time can execute this action on the same SO, -1 is no limit
	//float			Timeout;			// Pause between two action executions

	CSmartObjActionTpl() {}
	CSmartObjActionTpl(const Data::CParams& Desc);
};

}

#endif