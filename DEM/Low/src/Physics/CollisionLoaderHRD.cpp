#include "CollisionLoaderHRD.h"
#include <Physics/CollisionShape.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Math/SIMDMath.h>

namespace Resources
{

const DEM::Core::CRTTI& CCollisionLoaderHRD::GetResultType() const
{
	return Physics::CCollisionShape::RTTI;
}
//---------------------------------------------------------------------

// Can also load CDLODData resource and acceletare raycasting with minmax map
DEM::Core::PObject CCollisionLoaderHRD::CreateResource(CStrID UID)
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
	const rtm::vector4f Offset = Math::ToSIMD(Params.Get(sidOffset, vector3::Zero));
	const rtm::vector4f Scaling = Math::ToSIMD(Params.Get(sidScaling, vector3::One));
	if (Type == sidBox)
		return Physics::CCollisionShape::CreateBox(Math::ToSIMD(Params.Get(sidSize, vector3::Zero)), Offset, Scaling);
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
		// TODO: optimize, e.g. save convex hull in a .msh or simple binary file! May support both binary and HRD arrays!
		Data::PDataArray VerticesData;
		if (!Params.TryGet(VerticesData, sidVertices)) return nullptr;

		const auto VertexCount = VerticesData->GetCount();
		std::unique_ptr<vector3[]> Vertices(new vector3[VertexCount]);
		for (size_t i = 0; i < VertexCount; ++i)
			Vertices[i] = (*VerticesData)[i];

		return Physics::CCollisionShape::CreateConvexHull(Vertices.get(), VertexCount, Offset, Scaling);
	}

	return nullptr;
}
//---------------------------------------------------------------------

}
