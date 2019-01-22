#include "TextureLoaderCDLOD.h"

#include <Render/TextureData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

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

	void* pHeightData = nullptr;
	if (Stream->CanBeMapped()) pHeightData = Stream->Map();
	bool Mapped = !!pHeightData;
	if (!Mapped)
	{
		UPTR DataSize = HFWidth * HFHeight * sizeof(unsigned short);
		pHeightData = n_malloc(DataSize);
		if (Stream->Read(pHeightData, DataSize) != DataSize)
		{
			n_free(pHeightData);
			return NULL;
		}
	}

	Render::CTextureDesc TexDesc;
	TexDesc.Type = Render::Texture_2D;
	TexDesc.Width = HFWidth;
	TexDesc.Height = HFHeight;
	TexDesc.Depth = 1;
	TexDesc.ArraySize = 1;
	TexDesc.MipLevels = 1;
	TexDesc.MSAAQuality = Render::MSAA_None;
	TexDesc.Format = Render::PixelFmt_R16;

	Render::PTextureData TexData = n_new(Render::CTextureData);
	TexData->pData = pHeightData;
	TexData->Stream = Mapped ? Stream : nullptr;
	TexData->MipDataProvided = false;
	TexData->Desc = std::move(TexDesc);

	return TexData.Get();
}
//---------------------------------------------------------------------

}