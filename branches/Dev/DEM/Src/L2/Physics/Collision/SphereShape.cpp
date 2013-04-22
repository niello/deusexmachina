#include "SphereShape.h"

#include <Render/DebugDraw.h>

namespace Physics
{
ImplementRTTI(Physics::CSphereShape, Physics::CShape);
ImplementFactory(Physics::CSphereShape);

void CSphereShape::Init(Data::PParams Desc)
{
	InitialTfm.ident();
	InitialTfm.translate(vector3(
		Desc->Get<float>(CStrID("PosX")),
		Desc->Get<float>(CStrID("PosY")),
		Desc->Get<float>(CStrID("PosZ"))));
	SetTransform(InitialTfm);
	SetMaterialType(CMaterialTable::StringToMaterialType(Desc->Get<nString>(CStrID("Mtl"), "Metal")));
	SetRadius(Desc->Get<float>(CStrID("Radius"), 1.f));
}
//---------------------------------------------------------------------

bool CSphereShape::Attach(dSpaceID SpaceID)
{
	if (!CShape::Attach(SpaceID)) return false;
	dGeomID Sphere = dCreateSphere(0, Radius);
	AttachGeom(Sphere, SpaceID);
	if (MaterialType != InvalidMaterial)
	{
		dMassSetSphere(&ODEMass, CMaterialTable::GetDensity(MaterialType), Radius);
		TransformMass();
	}
	return true;
}
//---------------------------------------------------------------------

void CSphereShape::RenderDebug(const matrix44& ParentTfm)
{
	if (IsAttached())
		DebugDraw->DrawSphere(Transform.pos_component() + ParentTfm.pos_component(), Radius, GetDebugVisualizationColor());
}
//---------------------------------------------------------------------

} // namespace Physics
