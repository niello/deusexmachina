#include "BoxShape.h"

#include <Physics/rigidbody.h>
#include <gfx2/ngfxserver2.h>

namespace Physics
{
ImplementRTTI(Physics::CBoxShape, Physics::CShape);
ImplementFactory(Physics::CBoxShape);

void CBoxShape::Init(Data::PParams Desc)
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
	SetSize(vector3(
		Desc->Get<float>(CStrID("SizeX")),
		Desc->Get<float>(CStrID("SizeY")),
		Desc->Get<float>(CStrID("SizeZ"))));
}
//---------------------------------------------------------------------

// Create a box object, add it to ODE's collision space, and initialize the mass member.
bool CBoxShape::Attach(dSpaceID SpaceID)
{
	if (!CShape::Attach(SpaceID)) return false;
	dGeomID Box = dCreateBox(NULL, Size.x, Size.y, Size.z);
	AttachGeom(Box, SpaceID);
	if (MaterialType != InvalidMaterial)
	{
		dMassSetBox(&ODEMass, Physics::CMaterialTable::GetDensity(MaterialType), Size.x, Size.y, Size.z);
		TransformMass();
	}
	return true;
}
//---------------------------------------------------------------------

void CBoxShape::RenderDebug(const matrix44& ParentTfm)
{
	if (IsAttached())
	{
		matrix44 Tfm;
		Tfm.scale(Size);
		Tfm *= Transform;
		Tfm *= ParentTfm;
		nGfxServer2::Instance()->DrawShape(nGfxServer2::Box, Tfm, GetDebugVisualizationColor());
	}
}
//---------------------------------------------------------------------

} // namespace Physics
