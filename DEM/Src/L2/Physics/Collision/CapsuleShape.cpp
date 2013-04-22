#include "CapsuleShape.h"

#include <Render/DebugDraw.h>

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
		DebugDraw->DrawCapsule(Transform * ParentTfm, Radius, Length, GetDebugVisualizationColor());
}
//---------------------------------------------------------------------

} // namespace Physics
