#include "CapsuleShape.h"

#include <gfx2/ngfxserver2.h>

namespace Physics
{
ImplementRTTI(Physics::CCapsuleShape, Physics::CShape);
ImplementFactory(Physics::CCapsuleShape);

void CCapsuleShape::Init(Data::PParams Desc)
{
	InitialTfm.from_quaternion(quaternion(
		Desc->Get<float>(CStrID("RotX")),
		Desc->Get<float>(CStrID("RotY")),
		Desc->Get<float>(CStrID("RotZ")),
		Desc->Get<float>(CStrID("RotW"))));
	InitialTfm.translate(vector3(
		Desc->Get<float>(CStrID("PosX")),
		Desc->Get<float>(CStrID("PosY")),
		Desc->Get<float>(CStrID("PosZ"))));
	SetTransform(InitialTfm);
	SetMaterialType(CMaterialTable::StringToMaterialType(Desc->Get<nString>(CStrID("Mtl"), "Metal")));
	SetRadius(Desc->Get<float>(CStrID("Radius"), 1.f));
	SetLength(Desc->Get<float>(CStrID("Length"), 1.f));
}
//---------------------------------------------------------------------

bool CCapsuleShape::Attach(dSpaceID SpaceID)
{
	if (!CShape::Attach(SpaceID)) return false;
	dGeomID Capsule = dCreateCapsule(NULL, Radius, Length);
	AttachGeom(Capsule, SpaceID);
	if (MaterialType != InvalidMaterial)
	{
		dMassSetCapsule(&ODEMass, Physics::CMaterialTable::GetDensity(MaterialType), 3, Radius, Length);
		TransformMass();
	}
	return true;
}
//---------------------------------------------------------------------

void CCapsuleShape::RenderDebug(const matrix44& ParentTfm)
{
	if (IsAttached())
	{
		vector4 Color = GetDebugVisualizationColor();
		matrix44 WorldTfm = Transform * ParentTfm;

		matrix44 CapTfm;
		CapTfm.scale(vector3(Radius, Radius, Radius));
		CapTfm.set_translation(vector3(0.0f, 0.0f, Length * 0.5f));
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, CapTfm * WorldTfm, Color);
		CapTfm.pos_component().z -= Length;
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, CapTfm * WorldTfm, Color);

		matrix44 CylTfm;
		CylTfm.scale(vector3(Radius, Radius, Length));
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Cylinder, CylTfm * WorldTfm, Color);
	}
}
//---------------------------------------------------------------------

} // namespace Physics
