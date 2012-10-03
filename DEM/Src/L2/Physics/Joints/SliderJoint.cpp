#include "SliderJoint.h"

#include <Physics/RigidBody.h>

namespace Physics
{
ImplementRTTI(Physics::CSliderJoint, Physics::CJoint);
ImplementFactory(Physics::CSliderJoint);

CSliderJoint::CSliderJoint()
{
	AxisParams.Axis = vector3(1.0f, 0.0f, 0.0f);
}
//---------------------------------------------------------------------

void CSliderJoint::Init(PParams Desc)
{
	InitAxis(&AxisParams, Desc);
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added
// (happens in CJoint::Attach()) before joint transforms are set!!!
void CSliderJoint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateSlider(WorldID, GroupID);

	//???to some CJoiint::UtilFunc?
	if (AxisParams.IsLoStopEnabled)
		dJointSetSliderParam(ODEJointID, dParamLoStop, AxisParams.LoStop);
	if (AxisParams.IsHiStopEnabled)
		dJointSetSliderParam(ODEJointID, dParamHiStop, AxisParams.HiStop);
	dJointSetSliderParam(ODEJointID, dParamVel, AxisParams.Velocity);
	dJointSetSliderParam(ODEJointID, dParamFMax, AxisParams.FMax);
	dJointSetSliderParam(ODEJointID, dParamFudgeFactor, AxisParams.FudgeFactor);
	dJointSetSliderParam(ODEJointID, dParamBounce, AxisParams.Bounce);
	dJointSetSliderParam(ODEJointID, dParamCFM, AxisParams.CFM);
	dJointSetSliderParam(ODEJointID, dParamStopERP, AxisParams.StopERP);
	dJointSetSliderParam(ODEJointID, dParamStopCFM, AxisParams.StopCFM);

	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void CSliderJoint::UpdateTransform(const matrix44& Tfm)
{
	matrix33 m33(Tfm.x_component(), Tfm.y_component(), Tfm.z_component());
	vector3 a = m33 * AxisParams.Axis;
	dJointSetSliderAxis(ODEJointID, a.x, a.y, a.z);
}
//---------------------------------------------------------------------

} // namespace Physics
