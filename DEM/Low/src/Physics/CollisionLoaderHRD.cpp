#include "CollisionLoaderHRD.h"
#include <Physics/CollisionShape.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/Params.h>

namespace Resources
{

const Core::CRTTI& CCollisionLoaderHRD::GetResultType() const
{
	return Physics::CCollisionShape::RTTI;
}
//---------------------------------------------------------------------

// Can also load CDLODData resource and acceletare raycasting with minmax map
Core::PObject CCollisionLoaderHRD::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	Data::CParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;
	}

	const CStrID sidType("Type");
	const CStrID sidOffset("Offset");
	const CStrID sidSize("Size");
	const CStrID sidScaling("Scaling");
	const CStrID sidRadius("Radius");
	const CStrID sidHeight("Height");
	const CStrID sidBox("Box");
	const CStrID sidSphere("Sphere");
	const CStrID sidCapsuleX("CapsuleX");
	const CStrID sidCapsuleY("CapsuleY");
	const CStrID sidCapsuleZ("CapsuleZ");
	const CStrID sidConvexHull("ConvexHull");
	const CStrID sidVertices("Vertices");

	const auto Type = Params.Get(sidType, CStrID::Empty);
	const auto& Offset = Params.Get(sidOffset, vector3::Zero);
	const auto& Scaling = Params.Get(sidScaling, vector3::One);
	if (Type == sidBox)
		return Physics::CCollisionShape::CreateBox(Params.Get(sidSize, vector3::Zero), Offset, Scaling);
	else if (Type == sidSphere)
		return Physics::CCollisionShape::CreateSphere(Params.Get(sidRadius, 0.f), Offset, Scaling);
	else if (Type == sidCapsuleX)
		return Physics::CCollisionShape::CreateCapsuleX(Params.Get(sidRadius, 0.f), Params.Get(sidHeight, 0.f), Offset, Scaling);
	else if (Type == sidCapsuleY)
		return Physics::CCollisionShape::CreateCapsuleY(Params.Get(sidRadius, 0.f), Params.Get(sidHeight, 0.f), Offset, Scaling);
	else if (Type == sidCapsuleZ)
		return Physics::CCollisionShape::CreateCapsuleZ(Params.Get(sidRadius, 0.f), Params.Get(sidHeight, 0.f), Offset, Scaling);
	else if (Type == sidConvexHull)
	{
		NOT_IMPLEMENTED;
		//	return Physics::CCollisionShape::CreateConvexHull(Params.Get(sidVertices, Data::PDataArray{}), Offset, Scaling);
	}

	return nullptr;
}
//---------------------------------------------------------------------

}
