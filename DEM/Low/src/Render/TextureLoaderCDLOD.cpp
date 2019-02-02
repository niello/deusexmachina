#include "TextureLoaderCDLOD.h"

#include <Render/TextureData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>

namespace Resources
{

const Core::CRTTI& CTextureLoaderCDLOD::GetResultType() const
{
	return Render::CTextureData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CTextureLoaderCDLOD::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	// Only "HM" sub-id is supported
	if (strcmp("HM", pOutSubId)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'CDLD') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	U32 HFWidth, HFHeight;
	if (!Reader.Read(HFWidth)) return nullptr;
	if (!Reader.Read(HFHeight)) return nullptr;

	const UPTR SkipSize =
		sizeof(U32) + // PatchSize
		sizeof(U32) + // LODCount
		sizeof(U32) + // MinMaxDataSize
		sizeof(float) + // VerticalScale
		6 * sizeof(float); // AABB
	if (!Stream->Seek(SkipSize, IO::Seek_Current)) return nullptr;

	Data::PRAMData HeightData;
	if (Stream->CanBeMapped()) HeightData.reset(n_new(Data::CRAMDataMappedStream(Stream)));
	if (!HeightData->GetPtr()) // Not mapped
	{
		const UPTR DataSize = HFWidth * HFHeight * sizeof(unsigned short);
		HeightData.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
		if (Stream->Read(HeightData->GetPtr(), DataSize) != DataSize)
		{
			return nullptr;
		}
	}

	Render::PTextureData TexData = n_new(Render::CTextureData);

	Render::CTextureDesc& TexDesc = TexData->Desc;
	TexDesc.Type = Render::Texture_2D;
	TexDesc.Width = HFWidth;
	TexDesc.Height = HFHeight;
	TexDesc.Depth = 1;
	TexDesc.ArraySize = 1;
	TexDesc.MipLevels = 1;
	TexDesc.MSAAQuality = Render::MSAA_None;
	TexDesc.Format = Render::PixelFmt_R16;

	TexData->Data = std::move(HeightData);
	TexData->MipDataProvided = false;

	return TexData.Get();
}
//---------------------------------------------------------------------

}