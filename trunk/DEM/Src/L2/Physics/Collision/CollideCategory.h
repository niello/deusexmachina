#pragma once
#ifndef __DEM_L2_PHYS_COLLIDE_CAT_H__ //!!!to L1!
#define __DEM_L2_PHYS_COLLIDE_CAT_H__

// Collide category bits allow to define collision relationships between
// collision shapes (which collide shapes should be tested for collision
// against other collide shapes). For instance, it makes no sense
// to check for collision between static environment objects.
// (C) 2003 RadonLabs

namespace Physics
{

enum CollideCategory
{
	None = 0,
	Static = (1<<0),
	Dynamic = (1<<1),
	Trigger = (1<<2),
	All = 0xffffffff
};

};

#endif
