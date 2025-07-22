#include "TextureLoaderTGA.h"

#include <Render/TextureData.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/Buffer.h>

// Supports loading of TrueColor images only. //???support black-and-white too?
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

struct CTGA20Extension
{
	U16 Size;
	char AuthorName[41];
	char AuthorComments[324];
	U16 Month;
	U16 Day;
	U16 Year;
	U16 Hour;
	U16 Minute;
	U16 Second;
	char JobName[41];
	U16 JobHours;
	U16 JobMinutes;
	U16 JobSeconds;
	char SoftwareID[41];
	U16 SoftwareVersionNumber;
	char SoftwareVersionLetter;
	U32 KeyColor; // ARGB, A is the most significant byte
	U16 PixelRatioNumerator;
	U16 PixelRatioDenominator;
	U16 GammaNumerator;
	U16 GammaDenominator;
	U32 ColorCorrectionOffset;
	U32 PostageStampOffset;
	U32 ScanLineOffset;
	U8 AttributesType;
};

#pragma pack(pop)

// For CTGAHeader.ImageDescriptor bit checking
constexpr U8 TGA_RIGHT_TO_LEFT = (1 << 4);
constexpr U8 TGA_TOP_TO_BOTTOM = (1 << 5);
constexpr U8 TGA_ATTRIBUTE_BITS = 0x0f;

constexpr const char ReferenceSignature[] = "TRUEVISION-XFILE";

const DEM::Core::CRTTI& CTextureLoaderTGA::GetResultType() const
{
	return Render::CTextureData::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CTextureLoaderTGA::CreateResource(CStrID UID)
{
	ZoneScoped;
	ZoneText(UID.CStr(), strlen(UID.CStr()));

	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	CTGAHeader Header;
	if (!Reader.Read(Header)) return nullptr;

	// Only 32-bit TrueColor compressed and uncompressed images are supported for now
	if (Header.ImageType != 2 && Header.ImageType != 10) return nullptr;

	const bool IsRLECompressed = (Header.ImageType == 10);
	const bool FlipVertically = !(Header.ImageDescriptor & TGA_TOP_TO_BOTTOM);

	static_assert(sizeof(CTGAFooter) == 26);
	if (!Stream->Seek(26, IO::Seek_End)) return nullptr;
	CTGAFooter Footer;
	if (!Reader.Read(Footer)) return nullptr;

	// 32-bit TGA is alpha-enabled by default. For newer format versions
	// more reliable information can be extracted.
	bool HasAlpha = (Header.ImageDescriptor & TGA_ATTRIBUTE_BITS) || Header.BitsPerPixel == 32;
	if (!memcmp(Footer.Signature, ReferenceSignature, sizeof(ReferenceSignature) - 1))
	{
		// TGA 2.0 introduces this.
		// See https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
		if (Footer.ExtensionAreaOffset)
		{
			// Could read the whole extension area here but need only one byte, so don't bother. See below.
			//static_assert(sizeof(CTGA20Extension) == 495);
			//if (!Stream->Seek(Footer.ExtensionAreaOffset, IO::Seek_Begin)) return nullptr;
			//CTGA20Extension Extension;
			//if (!Reader.Read(Extension)) return nullptr;

			// Seek to "Attributes Type" - Field 24, 1 byte at pos 494 in an extension area
			if (!Stream->Seek(static_cast<I64>(Footer.ExtensionAreaOffset) + 494, IO::Seek_Begin)) return nullptr;

			U8 AttributesType;
			if (!Reader.Read(AttributesType)) return nullptr;

			// 4 for premultiplied
			HasAlpha = (AttributesType == 3 || AttributesType == 4);
		}
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

	const UPTR BytesPerPixel = (Header.BitsPerPixel >> 3);

	// NB: some algorithms below rely on this
	n_assert_dbg(BytesPerPixel <= BytesPerTargetPixel);

	if (!Stream->Seek(sizeof(Header) + Header.IDLength, IO::Seek_Begin)) return nullptr;

	Data::PBuffer Data;
	const UPTR RowSize = Header.ImageWidth * BytesPerTargetPixel;
	const UPTR DataSize = RowSize * Header.ImageHeight;
	if (IsRLECompressed)
	{
		ZoneScopedN("RLE");

		Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
		U8* pCurrPixel = static_cast<U8*>(Data->GetPtr());
		const U8* pDataEnd = pCurrPixel + DataSize;

		// We never expect TGA with more than 32 bpp
		n_assert_dbg(BytesPerPixel <= 4);

		while (pCurrPixel < pDataEnd)
		{
			U8 ChunkHeader;
			if (!Reader.Read(ChunkHeader)) return nullptr;

			if (ChunkHeader & 0x80) // RLE
			{
				// Read repeating pixel value
				U32 PixelValue;
				if (Stream->Read(&PixelValue, BytesPerPixel) != BytesPerPixel) return nullptr;

				// Copy the pixel into the destination buffer N times
				const U8 ChunkPixelCount = ChunkHeader - 127; // (ChunkHeader & 0x7f) + 1
				const U8* pChunkEnd = pCurrPixel + BytesPerTargetPixel * ChunkPixelCount;
				for (; pCurrPixel < pChunkEnd; pCurrPixel += BytesPerTargetPixel)
					std::memcpy(pCurrPixel, &PixelValue, BytesPerTargetPixel);
			}
			else // Raw
			{
				const UPTR ChunkSize = BytesPerTargetPixel * (ChunkHeader + 1);
				if (BytesPerPixel == BytesPerTargetPixel)
				{
					if (Stream->Read(pCurrPixel, ChunkSize) != ChunkSize) return nullptr;
					pCurrPixel += ChunkSize;
				}
				else
				{
					const U8* pChunkEnd = pCurrPixel + ChunkSize;
					for (; pCurrPixel < pChunkEnd; pCurrPixel += BytesPerTargetPixel)
						if (Stream->Read(pCurrPixel, BytesPerPixel) != BytesPerPixel) return nullptr;
				}
			}
		}

		// It is hard to flip during decompression, separate pass is much easier to implement
		if (FlipVertically)
		{
			ZoneScopedN("Flip");

			const UPTR EndRow = Header.ImageHeight / 2;
			for (UPTR Row = 0; Row < EndRow; ++Row)
			{
				U8* pRow1 = static_cast<U8*>(Data->GetPtr()) + Row * RowSize;
				U8* pRow2 = static_cast<U8*>(Data->GetPtr()) + (Header.ImageHeight - 1 - Row) * RowSize;
				std::swap_ranges(pRow1, pRow1 + RowSize, pRow2);
			}
		}
	}
	else
	{
		if (BytesPerPixel == BytesPerTargetPixel)
		{
			if (FlipVertically)
			{
				ZoneScopedN("Flip");

				// Only flipping required, read row by row
				Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
				auto* pDataStart = static_cast<char*>(Data->GetPtr());
				for (IPTR i = Header.ImageHeight - 1; i >= 0; --i)
					if (Stream->Read(pDataStart + (RowSize * i), RowSize) != RowSize) return nullptr;
			}
			else
			{
				ZoneScopedN("As-is");

				// No conversion needed, can use data as is. First try to map the stream.
				// If mapping not succeeded, copy data to new buffer.
				if (Stream->CanBeMapped()) Data.reset(n_new(Data::CBufferMappedStream(Stream)));
				if (!Data || !Data->GetPtr())
				{
					Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
					if (Stream->Read(Data->GetPtr(), DataSize) != DataSize) return nullptr;
				}
			}
		}
		else
		{
			ZoneScopedN("Per-pixel");

			Data.reset(n_new(Data::CBufferMallocAligned(DataSize, 16)));
			for (UPTR Row = 0; Row < Header.ImageHeight; ++Row)
			{
				U8* pCurrPixel = static_cast<U8*>(Data->GetPtr()) + (FlipVertically ? (Header.ImageHeight - 1 - Row) : Row) * RowSize;
				const U8* pRowEnd = pCurrPixel + Header.ImageWidth * BytesPerTargetPixel;
				for (; pCurrPixel < pRowEnd; pCurrPixel += BytesPerTargetPixel)
					if (Stream->Read(pCurrPixel, BytesPerPixel) != BytesPerPixel) return nullptr;
			}
		}
	}

	// If alpha is not present in a file but exists in a destination buffer, fill it with 255
	if (BytesPerTargetPixel == 4 && BytesPerPixel < 4)
	{
		ZoneScopedN("Fill alpha");

		U8* pCurrPixel = static_cast<U8*>(Data->GetPtr());
		const U8* pDataEnd = pCurrPixel + DataSize;
		for (; pCurrPixel < pDataEnd; pCurrPixel += 4)
			pCurrPixel[3] = 0xff;
	}

	Render::PTextureData TexData = n_new(Render::CTextureData);
	TexData->Data = std::move(Data);
	TexData->MipDataProvided = false;
	TexData->Desc = std::move(TexDesc);

	return TexData;
}
//---------------------------------------------------------------------

}
