#pragma once
#ifndef __DEM_L2_PHYSICS_RAGDOLL_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_RAGDOLL_H__

#include "Composite.h"
#include <Physics/Joints/JointAxis.h>

// A specialized physics composite, which binds physics joints contained
// in the composite to the character joints of a Nebula2 character.

namespace Physics
{

class CRagdoll: public CComposite
{
	__DeclareClass(CRagdoll);

private:

	// store backup info of physics pJoint bind poses
	struct CJointInfo
	{
		matrix44	Body1Matrix;	// transforms pJoint from model space to body1's space
		matrix44	Body2Matrix;	// transforms the pJoint from model space to body2's space
		float		AxisAngle1;		// angle around pJoint Axis 1 between body1 and body2
		float		AxisAngle2;		// angle around pJoint Axis 2 between body1 and body2
		CJointAxis	AxisParams1;	// axis1 parameters at bind time
		CJointAxis	AxisParams2;	// axis2 parameters at bind time
	};

	CJointInfo ComputeBindPoseInfoForJoint(CJoint* pJoint);
	float ComputeAxisAngle(const vector3& Anchor, const vector3& Axis, const vector3& p0, const vector3& p1);
	void FixJointStops(CJointAxis& jointAxis, const vector3& Anchor, const vector3& Body1Pos,
					   const vector3& Body2Pos, float BindAngle, const CJointAxis& BindJointAxis);

	nArray<CJointInfo> BindPoseInfo;

public:

	CRagdoll(): BindPoseInfo(24, 16) {}
	virtual ~CRagdoll();

	virtual void	Attach(dWorldID WorldID, dSpaceID DynamicSpaceID, dSpaceID StaticSpaceID);
	virtual void	Detach();
	void			Bind();
	void			WriteJoints();
	void			ReadJoints();
};

}

#endif
