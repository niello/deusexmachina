#include "TextureLoaderTGA.h"

#include <Render/TextureData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>

// Supports loading of TrueColor images only. //???support black-and-white too? RLE?
// Only a required subset is implemented, in accordance with a specification at:
//http://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf

namespace Resources
{
#pragma pack(push, 1)

struct CTGAHeader
{
	U8	IDLength;
	U8	ColorMapType;
	U8	ImageType;
	U16	ColorMapFirstEntryIndex;
	U16	ColorMapLength;
	U8	ColorMapEntrySize;
	U16	XOrigin;
	U16	YOrigin;
	U16	ImageWidth;
	U16	ImageHeight;
	U8	BitsPerPixel;
	U8	ImageDescriptor;
};

struct CTGAFooter
{
	U32	ExtensionAreaOffset;
	U32	DeveloperDirOffset;
	U8	Signature[18];
};

#pragma pack(pop)

// For CTGAHeader.ImageDescriptor bit checking
#define TGA_RIGHT_TO_LEFT	(1 << 4)
#define TGA_TOP_TO_BOTTOM	(1 << 5)
#define TGA_ATTRIBUTE_BITS	0x0f

constexpr const char ReferenceSignature[] = "TRUEVISION-XFILE";

const Core::CRTTI& CTextureLoaderTGA::GetResultType() const
{
	return Render::CTextureData::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CTextureLoaderTGA::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	CTGAHeader Header;
	if (!Reader.Read(Header)) return nullptr;

	// Only 32-bit TrueColor compressed and uncompressed images are supported for now
	if (Header.ImageType != 2 && Header.ImageType != 10) return nullptr;

	const bool IsRLECompressed = (Header.ImageType == 10);

	if (!Stream->Seek(26, IO::Seek_End)) return nullptr;
	CTGAFooter Footer;
	if (!Reader.Read(Footer)) return nullptr;

	bool HasAlpha;
	if (!memcmp(Footer.Signature, ReferenceSignature, sizeof(ReferenceSignature) - 1))
	{
		// New TGA
		if (Footer.ExtensionAreaOffset)
		{
			if (!Stream->Seek(Footer.ExtensionAreaOffset + 494, IO::Seek_Begin)) return nullptr;

			U8 AttributesType;
			if (!Reader.Read(AttributesType)) return nullptr;

			HasAlpha = (AttributesType == 3 || AttributesType == 4);
			//!!!???AttributesType == 4 is a premultiplied alpha. How to handle?
		}
		else HasAlpha = !!(Header.ImageDescriptor & TGA_ATTRIBUTE_BITS);
	}
	else
	{
		// Original TGA
		HasAlpha = !!(Header.ImageDescriptor & TGA_ATTRIBUTE_BITS);
	}

	Render::CTextureDesc TexDesc;
	TexDesc.Type = Render::Texture_2D;
	TexDesc.Width = Header.ImageWidth;
	TexDesc.Height = Header.ImageHeight;
	TexDesc.Depth = 1;
	TexDesc.ArraySize = 1;
	TexDesc.MipLevels = 1; //!!!set DesiredMipCount, if provided! some value for 'as in file', some for 'full chain' 
	TexDesc.MSAAQuality = Render::MSAA_None;

	UPTR BytesPerTargetPixel;
	switch (Header.BitsPerPixel)
	{
		case 32:
			TexDesc.Format = HasAlpha ? Render::PixelFmt_B8G8R8A8 : Render::PixelFmt_B8G8R8X8;
			BytesPerTargetPixel = 4;
			break;
		case 24:
			TexDesc.Format = Render::PixelFmt_B8G8R8X8;
			BytesPerTargetPixel = 4;
			break;
		//case 8:
		//	TexDesc.Format = greyscale;
		//	break;
		default:
			return nullptr;
	}

	UPTR BytesPerPixel = Header.BitsPerPixel >> 3;
	n_assert_dbg(BytesPerPixel <= BytesPerTargetPixel);

	if (!Stream->Seek(sizeof(Header) + Header.IDLength, IO::Seek_Begin)) return nullptr;

	Data::PRAMData Data;
	if (IsRLECompressed)
	{
		const UPTR DataSize = Header.ImageWidth * Header.ImageHeight * BytesPerTargetPixel;
		Data.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
		U8* pData = static_cast<U8*>(Data->GetPtr());
		U8* pCurrPixel = pData;
		const U8* pDataEnd = pData + DataSize;

		n_assert_dbg(BytesPerPixel <= 4); // We never expect TGA with more than 32 bpp
		while (pCurrPixel < pDataEnd)
		{
			U8 ChunkHeader;
			if (Stream->Read(&ChunkHeader, sizeof(U8)) != sizeof(U8)) return nullptr;

			if (ChunkHeader & 0x80)
			{
				// RLE
				U32 PixelValue;
				if (Stream->Read(&PixelValue, BytesPerPixel) != BytesPerPixel) return nullptr;

				const U8 ChunkPixelCount = ChunkHeader - 127; // (ChunkHeader & 0x7f) + 1
				if (BytesPerPixel == 4)
				{
					const U8* pChunkEnd = pCurrPixel + BytesPerTargetPixel * ChunkPixelCount;
					while (pCurrPixel < pChunkEnd)
					{
						*((U32*)pCurrPixel) = PixelValue;
						pCurrPixel += BytesPerTargetPixel;
					}
				}
				else
				{
					const U8* pChunkEnd = pCurrPixel + BytesPerTargetPixel * ChunkPixelCount;
					while (pCurrPixel < pChunkEnd)
					{
						pCurrPixel[0] = ((U8*)&PixelValue)[0];
						if (BytesPerPixel > 1)
						{
							pCurrPixel[1] = ((U8*)&PixelValue)[1];
							if (BytesPerPixel > 2)
							{
								pCurrPixel[2] = ((U8*)&PixelValue)[2];
							}
						}
						if (BytesPerTargetPixel > 3) pCurrPixel[3] = 0xff;
						pCurrPixel += BytesPerTargetPixel;
					}
				}
			}
			else
			{
				// Raw
				const U8 ChunkPixelCount = ChunkHeader + 1;
				if (BytesPerPixel == BytesPerTargetPixel)
				{
					const UPTR ChunkSize = BytesPerPixel * ChunkPixelCount;
					if (Stream->Read(pCurrPixel, ChunkSize) != ChunkSize) return nullptr;
					pCurrPixel += ChunkSize;
				}
				else
				{
					for (UPTR Curr = 0; Curr < ChunkPixelCount; ++Curr)
					{
						U32 PixelValue;
						if (Stream->Read(&PixelValue, BytesPerPixel) != BytesPerPixel) return nullptr;
						pCurrPixel[0] = ((U8*)&PixelValue)[0];
						if (BytesPerPixel > 1)
						{
							pCurrPixel[1] = ((U8*)&PixelValue)[1];
							if (BytesPerPixel > 2)
							{
								pCurrPixel[2] = ((U8*)&PixelValue)[2];
							}
						}
						if (BytesPerTargetPixel > 3) pCurrPixel[3] = 0xff;
						pCurrPixel += BytesPerTargetPixel;
					}
				}
			}
		}
	}
	else
	{
		if (BytesPerPixel == BytesPerTargetPixel)
		{
			// No conversion needed, can use data as is
			if (Stream->CanBeMapped()) Data.reset(n_new(Data::CRAMDataMappedStream(Stream)));
			if (!Data || !Data->GetPtr()) // Not mapped
			{
				const UPTR DataSize = Header.ImageWidth * Header.ImageHeight * BytesPerTargetPixel;
				Data.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
				if (Stream->Read(Data->GetPtr(), DataSize) != DataSize) return nullptr;
			}
		}
		else
		{
			const UPTR DataSize = Header.ImageWidth * Header.ImageHeight * BytesPerTargetPixel;
			Data.reset(n_new(Data::CRAMDataMallocAligned(DataSize, 16)));
			U8* pCurrPixel = static_cast<U8*>(Data->GetPtr());
			UPTR PixelCount = Header.ImageWidth * Header.ImageHeight;
			for (UPTR Curr = 0; Curr < PixelCount; ++Curr)
			{
				U32 PixelValue;
				if (Stream->Read(&PixelValue, BytesPerPixel) != BytesPerPixel) return nullptr;
				pCurrPixel[0] = ((U8*)&PixelValue)[0];
				if (BytesPerPixel > 1)
				{
					pCurrPixel[1] = ((U8*)&PixelValue)[1];
					if (BytesPerPixel > 2)
					{
						pCurrPixel[2] = ((U8*)&PixelValue)[2];
					}
				}
				if (BytesPerTargetPixel > 3) pCurrPixel[3] = 0xff;
				pCurrPixel += BytesPerTargetPixel;
			}
		}
	}

	Render::PTextureData TexData = n_new(Render::CTextureData);
	TexData->Data = std::move(Data);
	TexData->MipDataProvided = false;
	TexData->Desc = std::move(TexDesc);

	return TexData.Get();
}
//---------------------------------------------------------------------

}