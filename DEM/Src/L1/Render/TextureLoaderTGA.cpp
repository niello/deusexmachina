#include "TextureLoaderTGA.h"

#include <Render/Texture.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
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

bool CTextureLoaderTGA::Load(CResource& Resource)
{
	if (GPU.IsNullPtr()) FAIL;

	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_RANDOM)) FAIL;

	IO::CBinaryReader Reader(*File);

	CTGAHeader Header;
	if (!Reader.Read(Header)) FAIL;

	//???!!!add monochrome and RLE?!
	if (Header.ImageType != 2) FAIL;

	if (!File->Seek(26, IO::Seek_End)) FAIL;
	CTGAFooter Footer;
	if (!Reader.Read(Footer)) FAIL;

	bool HasAlpha;
	if (!memcmp(Footer.Signature, ReferenceSignature, sizeof(ReferenceSignature) - 1))
	{
		// New TGA
		if (Footer.ExtensionAreaOffset)
		{
			if (!File->Seek(Footer.ExtensionAreaOffset + 494, IO::Seek_Begin)) FAIL;

			U8 AttributesType;
			if (!Reader.Read(AttributesType)) FAIL;

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
			FAIL;
	}

	//	TexDesc.Format = DDSDX10FormatToPixelFormat(Header10.dxgiFormat);
	//	if (TexDesc.Format == Render::PixelFmt_Invalid) FAIL;

	if (!File->Seek(sizeof(Header) + Header.IDLength, IO::Seek_Begin)) FAIL;

	UPTR DataSize = (Header.ImageWidth * Header.ImageHeight * Header.BitsPerPixel) >> 3;

	////???!!!use mapped file instead?! at least if conversion not needed.
	void* pData = n_malloc(DataSize);
	if (File->Read(pData, DataSize) != DataSize) FAIL;

	Render::PTexture Texture = GPU->CreateTexture(TexDesc, Render::Access_GPU_Read, pData, false);

	//???!!!use mapped file instead?! at least if conversion not needed.
	n_free(pData);

	if (Texture.IsNullPtr()) FAIL;

	Resource.Init(Texture.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}