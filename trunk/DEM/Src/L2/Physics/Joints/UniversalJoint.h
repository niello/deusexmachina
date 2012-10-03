#pragma once
#ifndef __DEM_L2_PHYSICS_UNIVERSAL_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_UNIVERSAL_JOINT_H__

#include "Joint.h"
#include "JointAxis.h"
#include <util/nfixedarray.h>

// An universal joint. See ODE docs for details.

namespace Physics
{

class CUniversalJoint: public CJoint
{
	DeclareRTTI;
	DeclareFactory(CUniversalJoint);

public:

	vector3					Anchor;
	nFixedArray<CJointAxis>	AxisParams;

	CUniversalJoint();
	virtual ~CUniversalJoint() {}

	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void UpdateTransform(const matrix44& Tfm);
	virtual void RenderDebug();
};

RegisterFactory(CUniversalJoint);

}

#endif
