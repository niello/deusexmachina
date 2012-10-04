#include "Joint.h"

#include "JointAxis.h"

namespace Physics
{
ImplementRTTI(Physics::CJoint, Core::CRefCounted);

CJoint::~CJoint()
{
	if (IsAttached()) Detach();
}
//---------------------------------------------------------------------

void CJoint::InitAxis(CJointAxis* pAxis, PParams Desc)
{
	pAxis->Axis.x = Desc->Get<float>(CStrID("AxisX"));
	pAxis->Axis.y = Desc->Get<float>(CStrID("AxisY"));
	pAxis->Axis.z = Desc->Get<float>(CStrID("AxisZ"));

	int Idx = Desc->IndexOf(CStrID("LoStop"));
	pAxis->IsLoStopEnabled = (Idx != INVALID_INDEX);
	if (pAxis->IsLoStopEnabled) pAxis->LoStop = Desc->Get(Idx).GetValue<float>();
	
	Idx = Desc->IndexOf(CStrID("HiStop"));
	pAxis->IsHiStopEnabled = (Idx != INVALID_INDEX);
	if (pAxis->IsHiStopEnabled) pAxis->HiStop = Desc->Get(Idx).GetValue<float>();

	pAxis->Velocity = Desc->Get<float>(CStrID("Velocity"), 0.f);
	pAxis->FMax = Desc->Get<float>(CStrID("FMax"), 0.f);
	pAxis->FudgeFactor = Desc->Get<float>(CStrID("FudgeFactor"), 1.f);
	pAxis->Bounce = Desc->Get<float>(CStrID("Bounce"), 0.f);
	pAxis->CFM = Desc->Get<float>(CStrID("CFM"), 0.f);
	pAxis->StopERP = Desc->Get<float>(CStrID("StopERP"), 0.2f);
	pAxis->StopCFM = Desc->Get<float>(CStrID("StopCFM"), 0.f);
}
//---------------------------------------------------------------------

// Attach the joint to the world given a parent transformation. This will actually
// create the ODE joint, since ODE doesn't have a way to attach and detach joints.
void CJoint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	dJointAttach(ODEJointID,
		pBody1.isvalid() ? pBody1->GetODEBodyID() : NULL,
		pBody2.isvalid() ? pBody2->GetODEBodyID() : NULL);
}
//---------------------------------------------------------------------

void CJoint::Detach()
{
	n_assert(IsAttached());
	if (ODEJointID)
	{
		dJointDestroy(ODEJointID);
		ODEJointID = NULL;
	}
}
//---------------------------------------------------------------------

void CJoint::RenderDebug()
{
}
//---------------------------------------------------------------------

} // namespace Physics
