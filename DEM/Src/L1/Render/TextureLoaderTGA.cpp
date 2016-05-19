#include "TextureLoaderTGA.h"

#include <Render/Texture.h>
#include <Render/GPUDriver.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

// Supports loading of TrueColor images only. //???support black-and-white too? RLE?
// Only a required subset is implemented, in accordance with a specification at:
//http://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf

namespace Resources
{
__ImplementClass(Resources::CTextureLoaderTGA, 'TGAL', Resources::CTextureLoader);

#pragma pack(push, 1)

struct CTGAHeader
{
	U8	IDLength;
	U8	ColorMapType;
	U8	ImageType;
	U8	ColorMapSpecUnusedPart[5];
	U8	ImageSpecUnusedPart[4];
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

const char ReferenceSignature[] = "TRUEVISION-XFILE";

const Core::CRTTI& CTextureLoaderTGA::GetResultType() const
{
	return Render::CTexture::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CTextureLoaderTGA::Load(IO::CStream& Stream)
{
	if (GPU.IsNullPtr()) return NULL;

	IO::CBinaryReader Reader(Stream);

	CTGAHeader Header;
	if (!Reader.Read(Header)) return NULL;

	//???!!!add monochrome and RLE?!
	if (Header.ImageType != 2) return NULL;

	if (!Stream.Seek(26, IO::Seek_End)) return NULL;
	CTGAFooter Footer;
	if (!Reader.Read(Footer)) return NULL;

	bool HasAlpha;
	if (!memcmp(Footer.Signature, ReferenceSignature, sizeof(ReferenceSignature) - 1))
	{
		// New TGA
		if (Footer.ExtensionAreaOffset)
		{
			if (!Stream.Seek(Footer.ExtensionAreaOffset + 494, IO::Seek_Begin)) return NULL;

			U8 AttributesType;
			if (!Reader.Read(AttributesType)) return NULL;

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

	switch (Header.BitsPerPixel)
	{
		case 32:
			TexDesc.Format = HasAlpha ? Render::PixelFmt_B8G8R8A8 : Render::PixelFmt_B8G8R8X8;
			break;
		//???D3D11 24-bit support?
		//case 8:
		//	TexDesc.Format = greyscale;
		//	break;
		default:
			return NULL;
	}

	//	TexDesc.Format = DDSDX10FormatToPixelFormat(Header10.dxgiFormat);
	//	if (TexDesc.Format == Render::PixelFmt_Invalid) FAIL;

	if (!Stream.Seek(sizeof(Header) + Header.IDLength, IO::Seek_Begin)) return NULL;

	void* pData = NULL;
	if (Stream.CanBeMapped()) pData = Stream.Map();
	bool Mapped = !!pData;
	if (!Mapped)
	{
		UPTR DataSize = (Header.ImageWidth * Header.ImageHeight * Header.BitsPerPixel) >> 3;
		pData = n_malloc(DataSize);
		if (Stream.Read(pData, DataSize) != DataSize) return NULL;
	}

	Render::PTexture Texture = GPU->CreateTexture(TexDesc, Render::Access_GPU_Read, pData, false);

	if (Mapped) Stream.Unmap();
	else n_free(pData);

	return Texture.GetUnsafe();
}
//---------------------------------------------------------------------

}