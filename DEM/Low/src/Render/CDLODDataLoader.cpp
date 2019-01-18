#include "CDLODDataLoader.h"

#include <Render/GPUDriver.h>
#include <Render/CDLODData.h>
#include <Render/Texture.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CCDLODDataLoader::GetResultType() const
{
	return Render::CCDLODData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CCDLODDataLoader::CreateResource(CStrID UID)
{
	if (GPU.IsNullPtr()) return NULL;

	//!!!write R32F variant!
	if (!GPU->CheckCaps(Render::Caps_VSTex_R16)) return NULL;

	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'CDLD') return NULL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return NULL;

	Render::PCDLODData Obj = n_new(Render::CCDLODData);

	if (!Reader.Read(Obj->HFWidth)) return NULL;
	if (!Reader.Read(Obj->HFHeight)) return NULL;
	if (!Reader.Read(Obj->PatchSize)) return NULL;
	if (!Reader.Read(Obj->LODCount)) return NULL;
	U32 MinMaxDataSize = Reader.Read<U32>();
	if (!Reader.Read(Obj->VerticalScale)) return NULL;
	if (!Reader.Read(Obj->Box.Min.x)) return NULL;
	if (!Reader.Read(Obj->Box.Min.y)) return NULL;
	if (!Reader.Read(Obj->Box.Min.z)) return NULL;
	if (!Reader.Read(Obj->Box.Max.x)) return NULL;
	if (!Reader.Read(Obj->Box.Max.y)) return NULL;
	if (!Reader.Read(Obj->Box.Max.z)) return NULL;

	void* pHeightData = NULL;
	if (Stream->CanBeMapped()) pHeightData = Stream->Map();
	bool Mapped = !!pHeightData;
	if (!Mapped)
	{
		UPTR DataSize = Obj->HFWidth * Obj->HFHeight * sizeof(unsigned short);
		pHeightData = n_malloc(DataSize);
		if (Stream->Read(pHeightData, DataSize) != DataSize)
		{
			n_free(pHeightData);
			return NULL;
		}
	}

	Render::CTextureDesc TexDesc;
	TexDesc.Type = Render::Texture_2D;
	TexDesc.Width = Obj->HFWidth;
	TexDesc.Height = Obj->HFHeight;
	TexDesc.Depth = 1;
	TexDesc.ArraySize = 1;
	TexDesc.MipLevels = 1;
	TexDesc.MSAAQuality = Render::MSAA_None;
	TexDesc.Format = Render::PixelFmt_R16;

	Obj->HeightMap = GPU->CreateTexture(TexDesc, Render::Access_GPU_Read, pHeightData, false);

	if (Mapped) Stream.Unmap();
	else n_free(pHeightData);

	if (Obj->HeightMap.IsNullPtr()) return NULL;

	Obj->pMinMaxData = (I16*)n_malloc(MinMaxDataSize);
	Reader.GetStream().Read(Obj->pMinMaxData, MinMaxDataSize);

	UPTR PatchesW = (Obj->HFWidth - 1 + Obj->PatchSize - 1) / Obj->PatchSize;
	UPTR PatchesH = (Obj->HFHeight - 1 + Obj->PatchSize - 1) / Obj->PatchSize;
	UPTR Offset = 0;
	Obj->MinMaxMaps.SetSize(Obj->LODCount);
	for (UPTR LOD = 0; LOD < Obj->LODCount; ++LOD)
	{
		Render::CCDLODData::CMinMaxMap& MMMap = Obj->MinMaxMaps[LOD];
		MMMap.PatchesW = PatchesW;
		MMMap.PatchesH = PatchesH;
		MMMap.pData = Obj->pMinMaxData + Offset;
		Offset += PatchesW * PatchesH * 2;
		PatchesW = (PatchesW + 1) / 2;
		PatchesH = (PatchesH + 1) / 2;
	}

	UPTR TopPatchSize = Obj->PatchSize << (Obj->LODCount - 1);
	Obj->TopPatchCountW = (Obj->HFWidth - 1 + TopPatchSize - 1) / TopPatchSize;
	Obj->TopPatchCountH = (Obj->HFHeight - 1 + TopPatchSize - 1) / TopPatchSize;

	//???load normal map? now in material!

	return Obj.Get();
}
//---------------------------------------------------------------------

}