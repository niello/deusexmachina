#pragma once
#ifndef __DEM_L2_IAO_USE_ACTION_TPL_H__
#define __DEM_L2_IAO_USE_ACTION_TPL_H__

#include <Data/Flags.h>
#include <Core/RefCounted.h>

// Template record for IAO actions, either hardcoded (like CSmartObjActionPickItem)
// or custom (scripted CSmartObjActionCustom).

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
		FACE_OBJECT				= 0x01,	// If set, actor will face this object
		END_ON_DONE				= 0x02,	// If set, end action and stop animation when Duration passed
		RESET_ON_ABORT			= 0x04,	// If set, clear progress of unfinished action
		LOOP_ANIM				= 0x08,	// If set, action is done after Duration passed, anim looped, else done on anim done
		ACTOR_RADIUS_MATTERS	= 0x10	// If set, distance is adjusted by the actor radius
		// - disable after use
	};

	Data::CFlags	Flags;

	// Destination
	float			MinDistance;		// Can make union with navmesh region
	float			MaxDistance;		// < 0 = no distance requirements
	vector3			DestOffset;			// Offset from IAO position in local coords

	// Facing
	// - face dir

	// Animation and timing
	float			Duration;			// Only for LOOP_ANIM. Can split to min & max to randomize
	// - anim props or name or id
	// - fidget period min, max

	// Usage restrictions
	int				MaxUserCount;		// How much users at a time can execute this action on the same IAO
	//float			Timeout;			// Pause between two action executions

	CSmartObjActionTpl() {}
	CSmartObjActionTpl(const Data::CParams& Desc);
};

}

#endif