#include "TextureLoaderCDLOD.h"

#include <Render/TextureData.h>
#include <Render/CDLODData.h>
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

	n_assert_dbg(Version == 0x00020000);
	if (Version != 0x00020000) return nullptr;

	Render::CCDLODData::CHeader_0_2_0_0 Header;
	if (!Reader.Read(Header)) return nullptr;

	if (!Stream->Seek(Header.MinMaxDataCount * sizeof(I16), IO::Seek_Current)) return nullptr;

	Data::PBuffer Data;
	if (Stream->CanBeMapped()) Data.reset(n_new(Data::CBufferMappedStream(Stream)));
	if (!Data || !Data->GetPtr()) // Not mapped
	{
		const UPTR DataSize = Header.HFWidth * Header.HFHeight * sizeof(unsigned short);
		Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
		if (Stream->Read(Data->GetPtr(), DataSize) != DataSize)
		{
			return nullptr;
		}
	}

	Render::PTextureData TexData = n_new(Render::CTextureData);

	Render::CTextureDesc& TexDesc = TexData->Desc;
	TexDesc.Type = Render::Texture_2D;
	TexDesc.Width = Header.HFWidth;
	TexDesc.Height = Header.HFHeight;
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
