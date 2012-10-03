#include "UniversalJoint.h"

#include <gfx2/ngfxserver2.h>

namespace Physics
{
ImplementRTTI(Physics::CUniversalJoint, Physics::CJoint);
ImplementFactory(Physics::CUniversalJoint);

CUniversalJoint::CUniversalJoint(): AxisParams(2)
{
	AxisParams[0].Axis = vector3(0.0f, 0.0f, 1.0f);
	AxisParams[1].Axis = vector3(0.0f, 1.0f, 0.0f);
}
//---------------------------------------------------------------------

void CUniversalJoint::Init(PParams Desc)
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
void CUniversalJoint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateUniversal(WorldID, GroupID);

	for (int i = 0; i < 2; i++)
	{
		//???to some CJoiint::UtilFunc?
		const CJointAxis& CurrAxis = AxisParams[i];
		if (CurrAxis.IsLoStopEnabled)
			dJointSetUniversalParam(ODEJointID, dParamLoStop + dParamGroup * i, CurrAxis.LoStop);
		if (CurrAxis.IsHiStopEnabled)
			dJointSetUniversalParam(ODEJointID, dParamHiStop + dParamGroup * i, CurrAxis.HiStop);
		dJointSetUniversalParam(ODEJointID, dParamVel + dParamGroup * i, CurrAxis.Velocity);
		dJointSetUniversalParam(ODEJointID, dParamFMax + dParamGroup * i, CurrAxis.FMax);
		dJointSetUniversalParam(ODEJointID, dParamFudgeFactor + dParamGroup * i, CurrAxis.FudgeFactor);
		dJointSetUniversalParam(ODEJointID, dParamBounce + dParamGroup * i, CurrAxis.Bounce);
		dJointSetUniversalParam(ODEJointID, dParamCFM + dParamGroup * i, CurrAxis.CFM);
		dJointSetUniversalParam(ODEJointID, dParamStopERP + dParamGroup * i, CurrAxis.StopERP);
		dJointSetUniversalParam(ODEJointID, dParamStopCFM + dParamGroup * i, CurrAxis.StopCFM);
	}

	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void CUniversalJoint::UpdateTransform(const matrix44& Tfm)
{
	vector3 p = Tfm * Anchor;
	dJointSetUniversalAnchor(ODEJointID, p.x, p.y, p.z);

	matrix33 m33(Tfm.x_component(), Tfm.y_component(), Tfm.z_component());
	vector3 a0 = m33 * AxisParams[0].Axis;
	vector3 a1 = m33 * AxisParams[1].Axis;
	dJointSetUniversalAxis1(ODEJointID, a0.x, a0.y, a0.z);
	dJointSetUniversalAxis2(ODEJointID, a1.x, a1.y, a1.z);
}
//---------------------------------------------------------------------

void CUniversalJoint::RenderDebug()
{
	if (IsAttached())
	{
		matrix44 Tfm;
		dVector3 CurrAnchor;
		dJointGetUniversalAnchor(ODEJointID, CurrAnchor);
		Tfm.scale(vector3(0.1f, 0.1f, 0.1f));
		Tfm.translate(vector3(CurrAnchor[0], CurrAnchor[1], CurrAnchor[2]));
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, GetDebugVisualizationColor());
	}
}
//---------------------------------------------------------------------

} // namespace Physics
