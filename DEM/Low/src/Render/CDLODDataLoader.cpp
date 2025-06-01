#include "CDLODDataLoader.h"

#include <Render/CDLODData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const DEM::Core::CRTTI& CCDLODDataLoader::GetResultType() const
{
	return Render::CCDLODData::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CCDLODDataLoader::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'CDLD') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	n_assert_dbg(Version == 0x00020000);
	if (Version != 0x00020000) return nullptr;

	Render::CCDLODData::CHeader_0_2_0_0 Header;
	if (!Reader.Read(Header)) return nullptr;

	Render::PCDLODData Obj = n_new(Render::CCDLODData);
	Obj->HFWidth = Header.HFWidth;
	Obj->HFHeight = Header.HFHeight;
	Obj->PatchSize = Header.PatchSize;
	Obj->LODCount = Header.LODCount;
	Obj->VerticalScale = Header.VerticalScale;
	Obj->SplatMapUVCoeffs = Header.SplatMapUVCoeffs;

	Obj->pMinMaxData.reset(n_new_array(I16, Header.MinMaxDataCount));
	Reader.GetStream().Read(Obj->pMinMaxData.get(), Header.MinMaxDataCount * sizeof(I16));

	// Raster quad size is 1.0f units by default, control the final size with a scene node scaling
	const float RasterSizeX = static_cast<float>(Header.HFWidth - 1);
	const float RasterSizeZ = static_cast<float>(Header.HFHeight - 1);

	UPTR PatchesW = Math::DivCeil(Obj->HFWidth - 1, Obj->PatchSize);
	UPTR PatchesH = Math::DivCeil(Obj->HFHeight - 1, Obj->PatchSize);
	UPTR Offset = 0;
	Obj->MinMaxMaps.SetSize(Obj->LODCount);
	for (UPTR LOD = 0; LOD < Obj->LODCount; ++LOD)
	{
		Render::CCDLODData::CMinMaxMap& MMMap = Obj->MinMaxMaps[LOD];
		MMMap.PatchesW = PatchesW;
		MMMap.PatchesH = PatchesH;
		MMMap.pData = Obj->pMinMaxData.get() + Offset;
		Offset += PatchesW * PatchesH * 2;
		PatchesW = Math::DivCeil(PatchesW, 2);
		PatchesH = Math::DivCeil(PatchesH, 2);
	}
	n_assert2_dbg(PatchesW == 1 && PatchesH == 1, "CDLOD minmax map doesn't fold up into a single minmax pair!");

	// Coarsest LOD must be folded up to 1x1 and store global minmax pair
	float MinY, MaxY;
	Obj->GetMinMaxHeight(0, 0, Obj->LODCount - 1, MinY, MaxY);

	Obj->BoundsMax = rtm::vector_set(RasterSizeX, MaxY, RasterSizeZ);
	Obj->Box = Math::AABBFromMinMax(rtm::vector_set(0.f, MinY, 0.f), Obj->BoundsMax);

	return Obj.Get();
}
//---------------------------------------------------------------------

}
