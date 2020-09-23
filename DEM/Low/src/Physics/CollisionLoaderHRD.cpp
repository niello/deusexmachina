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
PResourceObject CCollisionLoaderHRD::CreateResource(CStrID UID)
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
	const CStrID sidRadius("Radius");
	const CStrID sidHeight("Height");
	const CStrID sidBox("Box");
	const CStrID sidSphere("Sphere");
	const CStrID sidCapsuleY("CapsuleY");

	const auto Type = Params.Get(sidType, CStrID::Empty);
	const auto Offset = Params.Get(sidOffset, vector3::Zero);
	if (Type == sidBox)
		return Physics::CCollisionShape::CreateBox(Params.Get(sidSize, vector3::Zero), Offset);
	else if (Type == sidSphere)
		return Physics::CCollisionShape::CreateSphere(Params.Get(sidRadius, 0.f), Offset);
	else if (Type == sidCapsuleY)
		return Physics::CCollisionShape::CreateCapsuleY(Params.Get(sidRadius, 0.f), Params.Get(sidHeight, 0.f), Offset);

	return nullptr;
}
//---------------------------------------------------------------------

}
