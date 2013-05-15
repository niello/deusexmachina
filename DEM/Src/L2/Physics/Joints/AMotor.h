#pragma once
#ifndef __DEM_L2_PHYSICS_AMOTOR_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_AMOTOR_H__

#include "Joint.h"
#include "JointAxis.h"
#include "util/nfixedarray.h"

// An angular motor joint. This will just constraint angles, not positions.
// Can be used together with a ball joint if additional position constraints are needed.

namespace Physics
{

class CAMotor: public CJoint
{
	__DeclareClass(CAMotor);

public:

	nFixedArray<CJointAxis> AxisParams;

	virtual ~CAMotor() {}

	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
    virtual void UpdateTransform(const matrix44& Tfm);

	void		SetNumAxes(uint Num) { n_assert(Num <= 3); AxisParams.SetSize(Num); }
	uint		GetNumAxes() const { return AxisParams.GetCount(); }
    void		UpdateVelocity(uint AxisIdx);
};

__RegisterClassInFactory(CAMotor);

}

#endif
