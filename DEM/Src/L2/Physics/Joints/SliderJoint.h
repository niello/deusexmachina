#pragma once
#ifndef __DEM_L2_PHYSICS_SLIDER_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_SLIDER_JOINT_H__

#include "Joint.h"
#include "JointAxis.h"

// A slider joint (see ODE docs for details).

namespace Physics
{

class CSliderJoint: public CJoint
{
	__DeclareClass(CSliderJoint);

public:

	CJointAxis AxisParams;

	CSliderJoint();
	virtual ~CSliderJoint() {}

	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void UpdateTransform(const matrix44& Tfm);
};

}

#endif
