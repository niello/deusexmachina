#include "SphereShape.h"

#include <gfx2/ngfxserver2.h>

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
	{
		matrix44 Tfm;
		Tfm.scale(vector3(Radius, Radius, Radius));
		Tfm *= Transform;
		Tfm *= ParentTfm;
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, GetDebugVisualizationColor());
	}
}
//---------------------------------------------------------------------

} // namespace Physics
