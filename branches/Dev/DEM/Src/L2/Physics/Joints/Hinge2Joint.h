#pragma once
#ifndef __DEM_L2_PHYSICS_HINGE2_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_HINGE2_JOINT_H__

#include "Joint.h"
#include "JointAxis.h"
#include <util/nfixedarray.h>

// A hinge2 joint. See ODE docs for details.

namespace Physics
{

class CHinge2Joint: public CJoint
{
	__DeclareClass(CHinge2Joint);

public:

	vector3					Anchor;
	float					SuspensionERP;
	float					SuspensionCFM;
	nFixedArray<CJointAxis>	AxisParams;

	CHinge2Joint();
	virtual ~CHinge2Joint() {}

	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void UpdateTransform(const matrix44& Tfm);
	virtual void RenderDebug();
};

}

#endif
