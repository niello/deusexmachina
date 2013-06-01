#pragma once
#ifndef __DEM_L2_PHYSICS_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_JOINT_H__

#include <Physics/RigidBodyOld.h>
#include <Data/Params.h>
#define dSINGLE
#include <ode/ode.h>

// A joint (also known as constraint) connects to rigid bodies. Subclasses
// of joint implement specific joint types.

namespace Physics
{
class CJointAxis;

class CJoint: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	dJointID	ODEJointID;
	PRigidBodyOld	pBody1;
	PRigidBodyOld	pBody2;

	static void	InitAxis(CJointAxis* pAxis, PParams Desc);
	vector4		GetDebugVisualizationColor() const { return vector4(1.0f, 0.0f, 1.0f, 1.0f); }

public:
	
	nString	LinkName;
	int		LinkIndex;

	CJoint(): ODEJointID(NULL) {}
	virtual ~CJoint() = 0;

	virtual void Init(PParams Desc) {}
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void Detach();
	virtual void UpdateTransform(const matrix44& Tfm) = 0;
	virtual void RenderDebug();

	bool IsAttached() const { return ODEJointID != NULL; }

	void				SetBodies(CRigidBodyOld* pRigidBody1, CRigidBodyOld* pRigidBody2);
	void				SetBody1(CRigidBodyOld* pBody);
	void				SetBody2(CRigidBodyOld* pBody);
	const CRigidBodyOld*	GetBody1() const { return pBody1.GetUnsafe(); }
	const CRigidBodyOld*	GetBody2() const { return pBody2.GetUnsafe(); }
	bool				IsLinkValid() const { return LinkName.IsValid(); }
	dJointID			GetJointId() const { return ODEJointID; }
};
//---------------------------------------------------------------------

typedef Ptr<CJoint> PJoint;

inline void CJoint::SetBodies(CRigidBodyOld* pRigidBody1, CRigidBodyOld* pRigidBody2)
{
	pBody1 = pRigidBody1;
	pBody2 = pRigidBody2;
}
//---------------------------------------------------------------------

inline void CJoint::SetBody1(CRigidBodyOld* pBody)
{
	pBody1 = pBody;
}
//---------------------------------------------------------------------

inline void CJoint::SetBody2(CRigidBodyOld* pBody)
{
	pBody2 = pBody;
}
//---------------------------------------------------------------------

}

#endif
