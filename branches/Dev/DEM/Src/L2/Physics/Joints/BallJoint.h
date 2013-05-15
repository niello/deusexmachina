#pragma once
#ifndef __DEM_L2_PHYSICS_BALL_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_BALL_JOINT_H__

#include "Joint.h"

// A ball-and-socket joint (see ODE docs for details).

namespace Physics
{

class CBallJoint: public CJoint
{
	__DeclareClass(CBallJoint);

public:

	vector3 Anchor;

	virtual ~CBallJoint() {}

	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& m);
	virtual void UpdateTransform(const matrix44& Tfm);
	virtual void RenderDebug();
};

__RegisterClassInFactory(CBallJoint);

}

#endif
