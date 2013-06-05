#include "AMotor.h"

#include <Data/DataArray.h>

namespace Physics
{
__ImplementClass(Physics::CAMotor, 'AMTR', Physics::CJoint);

void CAMotor::Init(Data::PParams Desc)
{
	Data::CDataArray& Axes = *Desc->Get<Data::PDataArray>(CStrID("Axes"));
	SetNumAxes(Axes.GetCount());
	for (int i = 0; i < Axes.GetCount(); i++)
		InitAxis(&AxisParams[i], Axes[i]);
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added (happens in CJoint::Attach())
// before joint transforms are set!!!
void CAMotor::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateAMotor(WorldID, GroupID);

	dJointSetAMotorMode(ODEJointID, dAMotorUser);
	dJointSetAMotorNumAxes(ODEJointID, AxisParams.GetCount());
	for (int i = 0; i < AxisParams.GetCount(); i++)
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
	n_assert((uint)AxisParams.GetCount() > AxisIdx);
	dJointSetAMotorParam(ODEJointID, dParamVel + dParamGroup * AxisIdx, AxisParams[AxisIdx].Velocity);
}
//---------------------------------------------------------------------

void CAMotor::UpdateTransform(const matrix44& Tfm)
{
	matrix33 m33(Tfm.AxisX(), Tfm.AxisY(), Tfm.AxisZ());
	for (int i = 0; i < AxisParams.GetCount(); i++)
	{
		vector3 a = m33 * AxisParams[i].Axis;
		dJointSetAMotorAxis(ODEJointID, i, 0, a.x, a.y, a.z);
	}
}
//---------------------------------------------------------------------

} // namespace Physics
