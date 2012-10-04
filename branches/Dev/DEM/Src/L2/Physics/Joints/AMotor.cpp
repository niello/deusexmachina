#include "AMotor.h"

#include <Data/DataArray.h>

namespace Physics
{
ImplementRTTI(Physics::CAMotor, Physics::CJoint);
ImplementFactory(Physics::CAMotor);

void CAMotor::Init(PParams Desc)
{
	CDataArray& Axes = *Desc->Get<PDataArray>(CStrID("Axes"));
	SetNumAxes(Axes.Size());
	for (int i = 0; i < Axes.Size(); i++)
		InitAxis(&AxisParams[i], Axes[i]);
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added (happens in CJoint::Attach())
// before joint transforms are set!!!
void CAMotor::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateAMotor(WorldID, GroupID);

	dJointSetAMotorMode(ODEJointID, dAMotorUser);
	dJointSetAMotorNumAxes(ODEJointID, AxisParams.Size());
	for (int i = 0; i < AxisParams.Size(); i++)
	{
		const CJointAxis& CurrAxis = AxisParams[i];
		dJointSetAMotorAngle(ODEJointID, i, CurrAxis.Angle);
		if (CurrAxis.IsLoStopEnabled)
			dJointSetAMotorParam(ODEJointID, dParamLoStop + dParamGroup * i, CurrAxis.LoStop);
		if (CurrAxis.IsHiStopEnabled)
			dJointSetAMotorParam(ODEJointID, dParamHiStop + dParamGroup * i, CurrAxis.HiStop);
		dJointSetAMotorParam(ODEJointID, dParamVel + dParamGroup * i, CurrAxis.Velocity);
		dJointSetAMotorParam(ODEJointID, dParamFMax + dParamGroup * i, CurrAxis.FMax);
		dJointSetAMotorParam(ODEJointID, dParamFudgeFactor + dParamGroup * i, CurrAxis.FudgeFactor);
		dJointSetAMotorParam(ODEJointID, dParamBounce + dParamGroup * i, CurrAxis.Bounce);
		dJointSetAMotorParam(ODEJointID, dParamCFM + dParamGroup * i, CurrAxis.CFM);
		dJointSetAMotorParam(ODEJointID, dParamStopERP + dParamGroup * i, CurrAxis.StopERP);
		dJointSetAMotorParam(ODEJointID, dParamStopCFM + dParamGroup * i, CurrAxis.StopCFM);
	}

	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void CAMotor::UpdateVelocity(uint AxisIdx)
{
	n_assert((uint)AxisParams.Size() > AxisIdx);
	dJointSetAMotorParam(ODEJointID, dParamVel + dParamGroup * AxisIdx, AxisParams[AxisIdx].Velocity);
}
//---------------------------------------------------------------------

void CAMotor::UpdateTransform(const matrix44& Tfm)
{
	matrix33 m33(Tfm.x_component(), Tfm.y_component(), Tfm.z_component());
	for (int i = 0; i < AxisParams.Size(); i++)
	{
		vector3 a = m33 * AxisParams[i].Axis;
		dJointSetAMotorAxis(ODEJointID, i, 0, a.x, a.y, a.z);
	}
}
//---------------------------------------------------------------------

} // namespace Physics
