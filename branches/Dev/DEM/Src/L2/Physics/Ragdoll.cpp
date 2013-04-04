#include "Ragdoll.h"

#include <Physics/Joints/BallJoint.h>
#include <Physics/Joints/HingeJoint.h>
#include <Physics/Joints/UniversalJoint.h>
#include <Physics/Joints/SliderJoint.h>
#include <Physics/Joints/Hinge2Joint.h>
#include <Physics/Level.h>

namespace Physics
{
ImplementRTTI(Physics::CRagdoll, Physics::CComposite);
ImplementFactory(Physics::CRagdoll);

CRagdoll::~CRagdoll()
{
	//SetCharacter(NULL);
}
//---------------------------------------------------------------------

void CRagdoll::Attach(dWorldID WorldID, dSpaceID DynamicSpaceID, dSpaceID StaticSpaceID)
{
	CComposite::Attach(WorldID, DynamicSpaceID, StaticSpaceID);

	// set new auto disable flags for rigid bodies
	for (int i = 0; i < Bodies.Size(); i++)
	{
		//???need to do it each activation?
		dBodyID BodyId = Bodies[i]->GetODEBodyID();
		dBodySetAutoDisableFlag(BodyId, 1);
		dBodySetAutoDisableSteps(BodyId, 2);
		dBodySetAutoDisableLinearThreshold(BodyId, 0.2f);
		dBodySetAutoDisableAngularThreshold(BodyId, 1.0f);
	}
}
//---------------------------------------------------------------------

void CRagdoll::Detach()
{
//	n_assert(pNCharacter);
	CComposite::Detach();
//	pNCharacter->animEnabled = true;
}
//---------------------------------------------------------------------

//???to utils?
// Computes the current angle of 2 bodies around a given Axis.
// Measures the angle on a 2d plane normal to the Axis
float CRagdoll::ComputeAxisAngle(const vector3& Anchor, const vector3& Axis, const vector3& p0, const vector3& p1)
{
	line3 AxisLine(Anchor, Anchor + Axis);
	vector3 vec0 = p0 - AxisLine.point(AxisLine.closestpoint(p0));
	vector3 vec1 = p1 - AxisLine.point(AxisLine.closestpoint(p1));
	float a = vector3::angle(vec0, vec1);
	return a;
}
//---------------------------------------------------------------------

// Returns a CJointInfo object which contains information about how the
// pJoint is positioned relative to its 2 bodies in the bind pose. The ragdoll
// must currently be in the bind pose for this method to work.
CRagdoll::CJointInfo CRagdoll::ComputeBindPoseInfoForJoint(CJoint* pJoint)
{
	n_assert(pJoint);
	CJointInfo JointInfo;
	JointInfo.AxisAngle1 = 0.0f;
	JointInfo.AxisAngle2 = 0.0f;
	vector3 Body1Pos;
	vector3 Body2Pos;
	if (pJoint->GetBody1()) Body1Pos = pJoint->GetBody1()->GetInitialTransform().pos_component();
	if (pJoint->GetBody2()) Body2Pos = pJoint->GetBody2()->GetInitialTransform().pos_component();

	//???!!!TO JOINT VIRTUAL FUNCTION!?
	// build a matrix which describes the joints position and orientation in the world
	matrix44 JointMatrix;
	if (pJoint->IsA(CBallJoint::RTTI))
	{
		JointMatrix.pos_component() = ((CBallJoint*)pJoint)->Anchor;
	}
	else if (pJoint->IsA(CHinge2Joint::RTTI))
	{
		CHinge2Joint* hinge2Joint = (CHinge2Joint*)pJoint;
		const vector3& Axis1 = hinge2Joint->AxisParams[0].Axis;
		const vector3& Axis2 = hinge2Joint->AxisParams[1].Axis;
		const vector3& Anchor = hinge2Joint->Anchor;
		const vector3& y = Axis1;
		const vector3& z = Axis2;
		JointMatrix.y_component() = y;
		JointMatrix.z_component() = z;
		JointMatrix.x_component() = y * z;
		JointMatrix.pos_component() = Anchor;
		JointInfo.AxisAngle1 = ComputeAxisAngle(Anchor, Axis1, Body1Pos, Body2Pos);
		JointInfo.AxisAngle2 = ComputeAxisAngle(Anchor, Axis2, Body1Pos, Body2Pos);
		JointInfo.AxisParams1 = hinge2Joint->AxisParams[0];
		JointInfo.AxisParams2 = hinge2Joint->AxisParams[1];
	}
	else if (pJoint->IsA(ÑHingeJoint::RTTI))
	{
		ÑHingeJoint* hingeJoint = (ÑHingeJoint*) pJoint;
		const vector3& Axis   = hingeJoint->AxisParams.Axis;
		const vector3& Anchor = hingeJoint->Anchor;
		const vector3& y = Axis;
		JointMatrix.y_component() = y;
		JointMatrix.x_component() = y * vector3(y.z, y.x, y.y);
		JointMatrix.z_component() = y * JointMatrix.x_component();
		JointMatrix.pos_component() = Anchor;
		JointInfo.AxisAngle1 = ComputeAxisAngle(Anchor, Axis, Body1Pos, Body2Pos);
		JointInfo.AxisParams1 = hingeJoint->AxisParams;
	}
	else if (pJoint->IsA(CSliderJoint::RTTI))
	{
		CSliderJoint* sliderJoint = (CSliderJoint*) pJoint;
		const vector3& Axis = sliderJoint->AxisParams.Axis;
		const vector3& x = Axis;
		JointMatrix.x_component() = x;
		JointMatrix.y_component() = x * vector3(x.z, x.x, x.y);
		JointMatrix.z_component() = x * JointMatrix.y_component();
		JointInfo.AxisAngle1 = ComputeAxisAngle(vector3(0.0f, 0.0f, 0.0f), Axis, Body1Pos, Body2Pos);
		JointInfo.AxisParams1 = sliderJoint->AxisParams;
	}
	else if (pJoint->IsA(CUniversalJoint::RTTI))
	{
		CUniversalJoint* uniJoint = (CUniversalJoint*) pJoint;
		const vector3& Axis1 = uniJoint->AxisParams[0].Axis;
		const vector3& Axis2 = uniJoint->AxisParams[1].Axis;
		const vector3& Anchor = uniJoint->Anchor;
		const vector3& z = Axis1;
		const vector3& y = Axis2;
		JointMatrix.z_component() = z;
		JointMatrix.y_component() = y;
		JointMatrix.x_component() = y * z;
		JointMatrix.pos_component() = Anchor;
		JointInfo.AxisAngle1 = ComputeAxisAngle(Anchor, Axis1, Body1Pos, Body2Pos);
		JointInfo.AxisAngle2 = ComputeAxisAngle(Anchor, Axis2, Body1Pos, Body2Pos);
		JointInfo.AxisParams1 = uniJoint->AxisParams[0];
		JointInfo.AxisParams2 = uniJoint->AxisParams[1];
	}

	// compute the matrix which transforms the joint from model space into it's pBody's space
	if (pJoint->GetBody1())
		JointInfo.Body1Matrix = JointMatrix * pJoint->GetBody1()->GetInvInitialTransform();
	if (pJoint->GetBody2())
		JointInfo.Body2Matrix = JointMatrix * pJoint->GetBody2()->GetInvInitialTransform();

	return JointInfo;
}
//---------------------------------------------------------------------

// This method binds the physics joints to the character joints by resolving
// the joint link name in the rigid bodies and physics joints into
// Nebula2 character joint indices. It also stores the difference matrices
// between the Nebula2 character bind pose and the physics bind pose.
void CRagdoll::Bind()
{
//	n_assert(pNCharacter);
	//const nCharSkeleton& Skeleton = pNCharacter->GetSkeleton();

	//// Convert joint names into indices
	//for (int i = 0; i < Bodies.Size(); i++)
	//{
	//	CRigidBody* pBody = Bodies[i];
	//	n_assert(pBody->IsLinkValid(CRigidBody::JointNode));
	//	pBody->LinkIndex = Skeleton.GetJointIndexByName(pBody->GetLinkName(CRigidBody::JointNode));
	//	if (pBody->LinkIndex == INVALID_INDEX)
	//		n_error("CRagdoll::Bind(): invalid joint name '%s'!", pBody->GetLinkName(CRigidBody::JointNode).Get());
	//}

	//for (int i = 0; i < Joints.Size(); i++)
	//	BindPoseInfo.Append(ComputeBindPoseInfoForJoint(Joints[i]));
}
//---------------------------------------------------------------------

// This method converts the current rigid body transformations into
// character joint transformations and writes them to the Nebula2 character.
// This method should be called after Composite::OnStepAfter() has been executed
// to ensure that all transformations are up to date.
void CRagdoll::WriteJoints()
{
	//n_assert(pNCharacter);
	//pNCharacter->animEnabled = false;
	//const nCharSkeleton& Skeleton = pNCharacter->GetSkeleton();

	//// get our inverse world space matrix
	//matrix44 InvRagdollWorld;
	//GetTransform().invert_simple(InvRagdollWorld);

	//for (int i = 0; i < Bodies.Size(); i++)
	//{
	//	CRigidBody* pBody = Bodies[i];
	//	if (pBody->IsLinkValid(CRigidBody::JointNode))
	//	{
	//		// last step - move the body's model space transform by the difference between the
	//		// rigid body's initial pose and the joint's bind pose. NOTE: this difference matrix
	//		// is constant and should only be computed once during setup
	//		nCharJoint& CharJoint = Skeleton.GetJointAt(pBody->LinkIndex);
	//		matrix44 BodyModel = pBody->GetTransform() * InvRagdollWorld;
	//		BodyModel.rotate_y(n_deg2rad(180.0f)); // Nebula2 FIX
	//		BodyModel = (CharJoint.GetPoseMatrix() * pBody->GetInvInitialTransform()) * BodyModel;
	//		CharJoint.SetMatrix(BodyModel);
	//	}
	//}
}
//---------------------------------------------------------------------

// Fix the joint stops for a joint axis that's going to be re-attached to the world.
void CRagdoll::FixJointStops(CJointAxis& CurrJointAxis, const vector3& Anchor,
							 const vector3& Body1Pos, const vector3& Body2Pos,
							 float BindAngle, const CJointAxis& BindJointAxis)
{
	// compute difference between current angle and bind pose angle
	float CurrAngle = ComputeAxisAngle(Anchor, CurrJointAxis.Axis, Body1Pos, Body2Pos);
	float Diff = CurrAngle - BindAngle;

	// add the difference to the joint stops
	if (CurrJointAxis.IsLoStopEnabled)
	{
		float LoStop = BindJointAxis.LoStop + Diff;
		if (CurrAngle < LoStop) LoStop = CurrAngle;
		CurrJointAxis.LoStop = LoStop;
	}

	if (CurrJointAxis.IsHiStopEnabled)
	{
		float HiStop = BindJointAxis.HiStop + Diff;
		if (CurrAngle > HiStop) HiStop = CurrAngle;
		CurrJointAxis.HiStop = HiStop;
	}
}
//---------------------------------------------------------------------

// Read joint positions from Nebula2 character and initialize the physics
// rigid body and joint positions.
// FIXME: would be nice to get linear and angular velocity info from joint,
// this could be extracted from the animation by the joint while it was alive.
void CRagdoll::ReadJoints()
{
	//n_assert(pNCharacter);
	//const nCharSkeleton& Skeleton = pNCharacter->GetSkeleton();

	//dWorldID ODEWorldID = PhysicsSrv->GetLevel()->GetODEWorldID();
	//dSpaceID ODESpaceID = PhysicsSrv->GetLevel()->GetODEDynamicSpaceID();

	//// Update rigid body positions
	//for (int i = 0; i < Bodies.Size(); i++)
	//{
	//	CRigidBody* pBody = Bodies[i];
	//	n_assert(pBody->IsLinkValid(CRigidBody::JointNode));
	//	const nCharJoint& CharJoint = Skeleton.GetJointAt(pBody->LinkIndex);

	//	pBody->Detach();

	//	// Compute the difference matrix between the pJoint's bind pose
	//	// and the rigid pBody's bind pose position
	//	matrix44 DiffMatrix = CharJoint.GetPoseMatrix() * pBody->GetInvInitialTransform();
	//	DiffMatrix.invert_simple(DiffMatrix);

	//	// Compute the new pBody's model space position from the current character joint matrix,
	//	// than translate body to world space
	//	matrix44 BodyModel = DiffMatrix * CharJoint.GetMatrix();
	//	pBody->Attach(ODEWorldID, ODESpaceID, BodyModel * GetTransform());
	//}

	//// Update joint positions and orientations
	//for (int i = 0; i < Joints.Size(); i++)
	//{
	//	const CJointInfo& JointInfo = BindPoseInfo[i];
	//	CJoint* pJoint = Joints[i];
	//	vector3 Body1Pos, Body2Pos;
	//	if (pJoint->GetBody1()) Body1Pos = pJoint->GetBody1()->GetTransform().pos_component();
	//	if (pJoint->GetBody2()) Body2Pos = pJoint->GetBody2()->GetTransform().pos_component();

	//	pJoint->Detach();

	//	matrix44 JointWorld = JointInfo.Body1Matrix * pJoint->GetBody1()->GetTransform();

	//	//???to virtual function?
	//	if (pJoint->IsA(CBallJoint::RTTI))
	//	{
	//		((CBallJoint*)pJoint)->Anchor = JointWorld.pos_component();
	//	}
	//	else if (pJoint->IsA(CHinge2Joint::RTTI))
	//	{
	//		CHinge2Joint* hinge2Joint = (CHinge2Joint*) pJoint;
	//		hinge2Joint->AxisParams[0].Axis = JointWorld.y_component();
	//		hinge2Joint->AxisParams[1].Axis = JointWorld.z_component();
	//		hinge2Joint->Anchor = JointWorld.pos_component();
	//		FixJointStops(hinge2Joint->AxisParams[0], hinge2Joint->Anchor, Body1Pos, Body2Pos, JointInfo.AxisAngle1, JointInfo.AxisParams1);
	//		FixJointStops(hinge2Joint->AxisParams[1], hinge2Joint->Anchor, Body1Pos, Body2Pos, JointInfo.AxisAngle2, JointInfo.AxisParams2);
	//	}
	//	else if (pJoint->IsA(ÑHingeJoint::RTTI))
	//	{
	//		ÑHingeJoint* hingeJoint = (ÑHingeJoint*) pJoint;
	//		hingeJoint->AxisParams.Axis = JointWorld.y_component();
	//		hingeJoint->Anchor = (JointWorld.pos_component());
	//		FixJointStops(hingeJoint->AxisParams, hingeJoint->Anchor, Body1Pos, Body2Pos, JointInfo.AxisAngle1, JointInfo.AxisParams1);
	//	}
	//	else if (pJoint->IsA(CSliderJoint::RTTI))
	//	{
	//		CSliderJoint* sliderJoint = (CSliderJoint*) pJoint;
	//		sliderJoint->AxisParams.Axis = JointWorld.x_component();
	//		FixJointStops(sliderJoint->AxisParams, vector3(0.0f, 0.0f, 0.0f), Body1Pos, Body2Pos, JointInfo.AxisAngle1, JointInfo.AxisParams1);
	//	}
	//	else if (pJoint->IsA(CUniversalJoint::RTTI))
	//	{
	//		CUniversalJoint* uniJoint = (CUniversalJoint*) pJoint;
	//		uniJoint->AxisParams[0].Axis = JointWorld.z_component();
	//		uniJoint->Anchor = (JointWorld.pos_component());
	//		FixJointStops(uniJoint->AxisParams[0], uniJoint->Anchor, Body1Pos, Body2Pos, JointInfo.AxisAngle1, JointInfo.AxisParams1);
	//		FixJointStops(uniJoint->AxisParams[1], uniJoint->Anchor, Body1Pos, Body2Pos, JointInfo.AxisAngle2, JointInfo.AxisParams2);
	//	}

	//	pJoint->Attach(ODEWorldID, 0, matrix44::identity);
	//}
}
//---------------------------------------------------------------------

} // namespace Physics
