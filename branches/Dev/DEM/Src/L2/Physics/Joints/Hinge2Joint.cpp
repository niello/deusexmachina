#include "Hinge2Joint.h"

namespace Physics
{
__ImplementClass(Physics::CHinge2Joint, 'JHN2', Physics::CJoint);

CHinge2Joint::CHinge2Joint():
	SuspensionERP(0.2f),
	SuspensionCFM(0.0f),
	AxisParams(2)
{
	AxisParams[0].Axis = vector3(0.0f, 1.0f, 0.0f);
	AxisParams[1].Axis = vector3(0.0f, 0.0f, 1.0f);
}
//---------------------------------------------------------------------

void CHinge2Joint::Init(PParams Desc)
{
	Anchor.x = Desc->Get<float>(CStrID("AnchorX"));
	Anchor.y = Desc->Get<float>(CStrID("AnchorY"));
	Anchor.z = Desc->Get<float>(CStrID("AnchorZ"));
	InitAxis(&AxisParams[0], Desc->Get<PParams>(CStrID("Axis0")));
	InitAxis(&AxisParams[1], Desc->Get<PParams>(CStrID("Axis1")));
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added
// (happens in CJoint::Attach()) before joint transforms are set!!!
void CHinge2Joint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateHinge2(WorldID, GroupID);

	for (int i = 0; i < 2; i++)
	{
		const CJointAxis& CurrAxis = AxisParams[i];
		if (CurrAxis.IsLoStopEnabled)
			dJointSetHinge2Param(ODEJointID, dParamLoStop + dParamGroup * i, CurrAxis.LoStop);
		if (CurrAxis.IsHiStopEnabled)
			dJointSetHinge2Param(ODEJointID, dParamHiStop + dParamGroup * i, CurrAxis.HiStop);
		dJointSetHinge2Param(ODEJointID, dParamVel + dParamGroup * i, CurrAxis.Velocity);
		dJointSetHinge2Param(ODEJointID, dParamFMax + dParamGroup * i, CurrAxis.FMax);
		dJointSetHinge2Param(ODEJointID, dParamFudgeFactor + dParamGroup * i, CurrAxis.FudgeFactor);
		dJointSetHinge2Param(ODEJointID, dParamBounce + dParamGroup * i, CurrAxis.Bounce);
		dJointSetHinge2Param(ODEJointID, dParamCFM + dParamGroup * i, CurrAxis.CFM);
		dJointSetHinge2Param(ODEJointID, dParamStopERP + dParamGroup * i, CurrAxis.StopERP);
		dJointSetHinge2Param(ODEJointID, dParamStopCFM + dParamGroup * i, CurrAxis.StopCFM);
	}
	dJointSetHinge2Param(ODEJointID, dParamSuspensionERP, SuspensionERP);
	dJointSetHinge2Param(ODEJointID, dParamSuspensionCFM, SuspensionCFM);

	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void CHinge2Joint::UpdateTransform(const matrix44& Tfm)
{
	vector3 p = Tfm * Anchor;
	dJointSetHinge2Anchor(ODEJointID, p.x, p.y, p.z);
	
	matrix33 m33(Tfm.AxisX(), Tfm.AxisY(), Tfm.AxisZ());
	vector3 a0 = m33 * AxisParams[0].Axis;
	vector3 a1 = m33 * AxisParams[1].Axis;
	dJointSetHinge2Axis1(ODEJointID, a0.x, a0.y, a0.z);
	dJointSetHinge2Axis2(ODEJointID, a1.x, a1.y, a1.z);
}
//---------------------------------------------------------------------

void CHinge2Joint::RenderDebug()
{
	//GFX
	/*
	if (IsAttached())
	{
		matrix44 Tfm;
		dVector3 CurrAnchor;
		dJointGetHinge2Anchor(ODEJointID, CurrAnchor);
		Tfm.scale(vector3(0.1f, 0.1f, 0.1f));
		Tfm.translate(vector3(CurrAnchor[0], CurrAnchor[1], CurrAnchor[2]));
		DebugDraw->DrawSphere(Tfm, GetDebugVisualizationColor());
	}
	*/
}
//---------------------------------------------------------------------

} // namespace Physics

