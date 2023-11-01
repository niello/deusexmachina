#include "CDLODDataLoader.h"

#include <Render/CDLODData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CCDLODDataLoader::GetResultType() const
{
	return Render::CCDLODData::RTTI;
}
//---------------------------------------------------------------------

Core::PObject CCDLODDataLoader::CreateResource(CStrID UID)
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

	Render::CCDLODData::CHeader_0_2_0_0 Header;
	if (!Reader.Read(Header)) return nullptr;

	Render::PCDLODData Obj = n_new(Render::CCDLODData);
	Obj->HFWidth = Header.HFWidth;
	Obj->HFHeight = Header.HFHeight;
	Obj->PatchSize = Header.PatchSize;
	Obj->LODCount = Header.LODCount;
	Obj->SplatMapUVCoeffs = Header.SplatMapUVCoeffs;

	Obj->pMinMaxData.reset(n_new_array(I16, Header.MinMaxDataCount));
	Reader.GetStream().Read(Obj->pMinMaxData.get(), Header.MinMaxDataCount * sizeof(I16));

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
	I16 MinY, MaxY;
	Obj->GetMinMaxHeight(0, 0, Obj->LODCount - 1, MinY, MaxY);

	Obj->Box.Min.x = 0.f;
	Obj->Box.Min.y = static_cast<float>(MinY);
	Obj->Box.Min.z = 0.f;
	Obj->Box.Max.x = static_cast<float>(Obj->HFWidth - 1);
	Obj->Box.Max.y = static_cast<float>(MaxY);
	Obj->Box.Max.z = static_cast<float>(Obj->HFHeight - 1);

	return Obj.Get();
}
//---------------------------------------------------------------------

}
