#include "TextureLoaderCDLOD.h"

#include <Render/TextureData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/Buffer.h>

namespace Resources
{

const Core::CRTTI& CTextureLoaderCDLOD::GetResultType() const
{
	return Render::CTextureData::RTTI;
}
//---------------------------------------------------------------------

Core::PObject CTextureLoaderCDLOD::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

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

	Data::PBuffer Data;
	if (Stream->CanBeMapped()) Data.reset(n_new(Data::CBufferMappedStream(Stream)));
	if (!Data || !Data->GetPtr()) // Not mapped
	{
		const UPTR DataSize = HFWidth * HFHeight * sizeof(unsigned short);
		Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
		if (Stream->Read(Data->GetPtr(), DataSize) != DataSize)
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

	TexData->Data = std::move(Data);
	TexData->MipDataProvided = false;

	return TexData.Get();
}
//---------------------------------------------------------------------

}
