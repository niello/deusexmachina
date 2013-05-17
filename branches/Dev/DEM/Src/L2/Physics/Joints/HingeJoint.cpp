#include "HingeJoint.h"

namespace Physics
{
__ImplementClass(Physics::ÑHingeJoint, 'JHNG', Physics::CJoint);

ÑHingeJoint::ÑHingeJoint()
{
	AxisParams.Axis = vector3(0.0f, 1.0f, 0.0f);
}
//---------------------------------------------------------------------

void ÑHingeJoint::Init(PParams Desc)
{
	Anchor.x = Desc->Get<float>(CStrID("AnchorX"));
	Anchor.y = Desc->Get<float>(CStrID("AnchorY"));
	Anchor.z = Desc->Get<float>(CStrID("AnchorZ"));
	InitAxis(&AxisParams, Desc);
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added
// (happens in CJoint::Attach()) before joint transforms are set!!!
void ÑHingeJoint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateHinge(WorldID, GroupID);

	//???to some CJoiint::UtilFunc?
	if (AxisParams.IsLoStopEnabled)
		dJointSetHingeParam(ODEJointID, dParamLoStop, AxisParams.LoStop);
	if (AxisParams.IsHiStopEnabled)
		dJointSetHingeParam(ODEJointID, dParamHiStop, AxisParams.HiStop);
	dJointSetHingeParam(ODEJointID, dParamVel, AxisParams.Velocity);
	dJointSetHingeParam(ODEJointID, dParamFMax, AxisParams.FMax);
	dJointSetHingeParam(ODEJointID, dParamFudgeFactor, AxisParams.FudgeFactor);
	dJointSetHingeParam(ODEJointID, dParamBounce, AxisParams.Bounce);
	dJointSetHingeParam(ODEJointID, dParamCFM, AxisParams.CFM);
	dJointSetHingeParam(ODEJointID, dParamStopERP, AxisParams.StopERP);
	dJointSetHingeParam(ODEJointID, dParamStopCFM, AxisParams.StopCFM);

	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void ÑHingeJoint::UpdateTransform(const matrix44& Tfm)
{
	vector3 p = Tfm * Anchor;
	dJointSetHingeAnchor(ODEJointID, p.x, p.y, p.z);

	matrix33 m33(Tfm.AxisX(), Tfm.AxisY(), Tfm.AxisZ());
	vector3 a = m33 * AxisParams.Axis;
	dJointSetHingeAxis(ODEJointID, a.x, a.y, a.z);
}
//---------------------------------------------------------------------

void ÑHingeJoint::RenderDebug()
{
	//GFX
	/*
	if (IsAttached())
	{
		matrix44 Tfm;
		dVector3 CurrAnchor;
		dJointGetHingeAnchor(ODEJointID, CurrAnchor);
		Tfm.scale(vector3(0.1f, 0.1f, 0.1f));
		Tfm.translate(vector3(CurrAnchor[0], CurrAnchor[1], CurrAnchor[2]));
		DebugDraw->DrawSphere(Tfm, GetDebugVisualizationColor());
	}
	*/
}
//---------------------------------------------------------------------

} // namespace Physics
