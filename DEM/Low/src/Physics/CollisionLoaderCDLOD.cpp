#include "CollisionLoaderCDLOD.h"
#include <Physics/HeightfieldShape.h>
#include <Render/CDLODData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const DEM::Core::CRTTI& CCollisionLoaderCDLOD::GetResultType() const
{
	return Physics::CCollisionShape::RTTI;
}
//---------------------------------------------------------------------

// Can also load CDLODData resource and acceletare raycasting with minmax map
DEM::Core::PObject CCollisionLoaderCDLOD::CreateResource(CStrID UID)
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
	if (Version != 0x00020000) return nullptr;

	Render::CCDLODData::CHeader_0_2_0_0 Header;
	if (!Reader.Read(Header)) return nullptr;

	// Heightfield must have an area
	if (Header.HFWidth < 2 || Header.HFHeight < 2) return nullptr;

	// Skip minmax data to the root minmax pair
	if (!Stream->Seek((Header.MinMaxDataCount - 2) * sizeof(I16), IO::Seek_Current)) return nullptr;

	// Read global minmax
	I16 MinY, MaxY;
	if (!Reader.Read(MinY)) return nullptr;
	if (!Reader.Read(MaxY)) return nullptr;
	const auto MinYF = static_cast<float>(MinY) * Header.VerticalScale;
	const auto MaxYF = static_cast<float>(MaxY) * Header.VerticalScale;

	// Bullet shape is created with an origin at the center of a heightmap AABB.
	// Calculate an offset between that center and the real origin, which is a half extent of the terrain.
	const rtm::vector4f Offset = rtm::vector_set(Header.HFWidth * 0.5f, (MinYF + MaxYF) * 0.5f, Header.HFHeight * 0.5f);

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

	auto pBtShape = new btDEMHeightfieldTerrainShape(Header.HFWidth, Header.HFHeight, Data.get(), Header.VerticalScale, MinYF, MaxYF, 1, PHY_SHORT, false);

	return new Physics::CHeightfieldShape(pBtShape, std::move(Data), Offset);
}
//---------------------------------------------------------------------

}
