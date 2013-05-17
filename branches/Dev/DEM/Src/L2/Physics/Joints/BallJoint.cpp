#include "BallJoint.h"

namespace Physics
{
__ImplementClass(Physics::CBallJoint, 'JBAL', Physics::CJoint);

void CBallJoint::Init(PParams Desc)
{
	Anchor.x = Desc->Get<float>(CStrID("AnchorX"));
	Anchor.y = Desc->Get<float>(CStrID("AnchorY"));
	Anchor.z = Desc->Get<float>(CStrID("AnchorZ"));
}
//---------------------------------------------------------------------

// NOTE: it is important that rigid bodies are added
// (happens in CJoint::Attach()) before joint transforms are set!!!
void CBallJoint::Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm)
{
	ODEJointID = dJointCreateBall(WorldID, GroupID);
	CJoint::Attach(WorldID, GroupID, ParentTfm);
	UpdateTransform(ParentTfm);
}
//---------------------------------------------------------------------

void CBallJoint::UpdateTransform(const matrix44& Tfm)
{
	vector3 a = Tfm * Anchor;
	dJointSetBallAnchor(ODEJointID, a.x, a.y, a.z);
}
//---------------------------------------------------------------------

void CBallJoint::RenderDebug()
{
	//GFX
	/*
	if (IsAttached())
	{
		matrix44 Tfm;
		dVector3 CurrAnchor;
		dJointGetBallAnchor(ODEJointID, CurrAnchor);
		Tfm.scale(vector3(0.1f, 0.1f, 0.1f));
		Tfm.translate(vector3(CurrAnchor[0], CurrAnchor[1], CurrAnchor[2]));
		DebugDraw->DrawSphere(Tfm, GetDebugVisualizationColor());
	}
	*/
}
//---------------------------------------------------------------------

} // namespace Physics
