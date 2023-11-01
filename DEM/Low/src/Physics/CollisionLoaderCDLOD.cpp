#include "CollisionLoaderCDLOD.h"
#include <Physics/HeightfieldShape.h>
#include <Render/CDLODData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Resources
{

const Core::CRTTI& CCollisionLoaderCDLOD::GetResultType() const
{
	return Physics::CCollisionShape::RTTI;
}
//---------------------------------------------------------------------

// Can also load CDLODData resource and acceletare raycasting with minmax map
Core::PObject CCollisionLoaderCDLOD::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	// Only "Collision" sub-id is supported
	if (strcmp("Collision", pOutSubId)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'CDLD') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;
	n_assert_dbg(Version == 0x00020000);

	Render::CCDLODData::CHeader_0_2_0_0 Header;
	if (!Reader.Read(Header)) return nullptr;

	// Heightfield must have an area
	if (Header.HFWidth < 2 || Header.HFHeight < 2) return nullptr;

	//!!!DBG TMP!
	return nullptr;

	/*
	// Bullet shape is created with an origin at the center of a heightmap AABB.
	// Calculate an offset between that center and the real origin.
	float MinX, MaxX, MinZ, MaxZ, MinY, MaxY;
	if (!Reader.Read(MinX)) return nullptr;
	if (!Reader.Read(MaxX)) return nullptr;
	if (!Reader.Read(MinZ)) return nullptr;
	if (!Reader.Read(MaxZ)) return nullptr;
	if (!Reader.Read(MinY)) return nullptr;
	if (!Reader.Read(MaxY)) return nullptr;

	//// NB: Y must be offset differently, it is not a mistake!
	//vector3 Offset((MaxX - MinX) * 0.5f, (MinY + MaxY) * 0.5f, (MaxZ - MinZ) * 0.5f);

	const UPTR DataSize = Header.HFWidth * Header.HFHeight * sizeof(short);
	Physics::PHeightfieldData Data(new char[DataSize]);
	if (Stream->Read(Data.get(), DataSize) != DataSize) return nullptr;

	// Convert unsigned shorts to signed
	I16* pCurr = reinterpret_cast<short*>(Data.get());
	const I16* pEnd = reinterpret_cast<const short*>(Data.get() + DataSize);
	for (; pCurr < pEnd; ++pCurr)
	{
		const I32 Value = static_cast<U16>(*pCurr) - 32768;
		*pCurr = static_cast<I16>(Value);
	}

	btHeightfieldTerrainShape* pBtShape =
		new btHeightfieldTerrainShape(Header.HFWidth, Header.HFHeight, Data.get(), VerticalScale, MinY, MaxY, 1, PHY_SHORT, false);

	// Convert W & H from heightmap samples to 3D units
	btVector3 LocalScaling((MaxX - MinX) / static_cast<float>(HFWidth - 1), 1.f, (MaxZ - MinZ) / static_cast<float>(HFHeight - 1));
	pBtShape->setLocalScaling(LocalScaling);

	return n_new(Physics::CHeightfieldShape(pBtShape, std::move(Data), Offset));
	*/
}
//---------------------------------------------------------------------

}
