#include "TextureLoaderDDS.h"

#include <Render/Texture.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

// DDS code and definitions are based on:
// https://github.com/Microsoft/DirectXTex/blob/master/DDSTextureLoader/DDSTextureLoader.cpp

const U32 DDS_MAGIC = 0x20534444; // "DDS "

#ifndef MAKEFOURCC
	#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
				((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | \
				((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#define DEM_MAX_MIP_LEVELS		(15)		// D3D11_REQ_MIP_LEVELS

#define DDS_FOURCC				0x00000004	// DDPF_FOURCC
#define DDS_RGB					0x00000040	// DDPF_RGB
#define DDS_LUMINANCE			0x00020000	// DDPF_LUMINANCE
#define DDS_ALPHA				0x00000002	// DDPF_ALPHA
#define DDS_BUMPDUDV			0x00080000	// DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME	0x00800000	// DDSD_DEPTH

#define DDS_HEIGHT				0x00000002	// DDSD_HEIGHT
#define DDS_WIDTH				0x00000004	// DDSD_WIDTH

#define DDS_CUBEMAP				0x00000200	// DDSCAPS2_CUBEMAP
#define DDS_CUBEMAP_POSITIVEX	0x00000600	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX	0x00000a00	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY	0x00001200	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY	0x00002200	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ	0x00004200	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ	0x00008200	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES	(DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
								DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
								DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define ISBITMASK(ddpf, r, g, b, a) (ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a)

enum DDS_MISC_FLAGS2
{
	DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

enum
{
	DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
	//DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
	//DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
	//DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	DXGI_FORMAT_B5G6R5_UNORM                = 85,
	DXGI_FORMAT_BC1_UNORM                   = 71,
	//DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
	DXGI_FORMAT_BC2_UNORM                   = 74,
	//DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
	DXGI_FORMAT_BC3_UNORM                   = 77,
	//DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
	DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
	DXGI_FORMAT_D32_FLOAT                   = 40,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	DXGI_FORMAT_UNKNOWN	                    = 0
};

enum
{
	D3D11_RESOURCE_DIMENSION_TEXTURE1D	= 2,
	D3D11_RESOURCE_DIMENSION_TEXTURE2D	= 3,
	D3D11_RESOURCE_DIMENSION_TEXTURE3D	= 4
};

enum
{
	D3D11_RESOURCE_MISC_TEXTURECUBE	= 0x4L,
};

#pragma pack(push, 1)

struct DDS_PIXELFORMAT
{
	U32				size;
	U32				flags;
	U32				fourCC;
	U32				RGBBitCount;
	U32				RBitMask;
	U32				GBitMask;
	U32				BBitMask;
	U32				ABitMask;
};

struct DDS_HEADER
{
	U32				size;
	U32				flags;
	U32				height;
	U32				width;
	U32				pitchOrLinearSize;
	U32				depth;				// only if DDS_HEADER_FLAGS_VOLUME is set in flags
	U32				mipMapCount;
	U32				reserved1[11];
	DDS_PIXELFORMAT	ddspf;
	U32				caps;
	U32				caps2;
	U32				caps3;
	U32				caps4;
	U32				reserved2;
};

struct DDS_HEADER_DXT10
{
	U32				dxgiFormat;
	U32				resourceDimension;
	U32				miscFlag;			// see D3D11_RESOURCE_MISC_FLAG
	U32				arraySize;
	U32				miscFlags2;
};

#pragma pack(pop)

namespace Resources
{
__ImplementClass(Resources::CTextureLoaderDDS, 'DDSL', Resources::CTextureLoader);

//!!!need sRGB handling!
Render::EPixelFormat DDSFormatToPixelFormat(const DDS_PIXELFORMAT& DDSFormat)
{
	if (DDSFormat.flags & DDS_RGB)
	{
		switch (DDSFormat.RGBBitCount)
		{
			case 32:
			{
				if (ISBITMASK(DDSFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
					return Render::PixelFmt_R8G8B8A8;
				if (ISBITMASK(DDSFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
					return Render::PixelFmt_B8G8R8A8;
				if (ISBITMASK(DDSFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
					return Render::PixelFmt_B8G8R8X8;
				//if (ISBITMASK(DDSFormat, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
				//	return DXGI_FORMAT_R32_FLOAT;

				//// Note that many common DDS reader/writers (including D3DX) swap the
				//// the RED/BLUE masks for 10:10:10:2 formats. We assume
				//// below that the 'backwards' header mask is being used since it is most
				//// likely written by D3DX. The more robust solution is to use the 'DX10'
				//// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly	
				//// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
				//if (ISBITMASK(DDSFormat, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
				//	return DXGI_FORMAT_R10G10B10A2_UNORM;

				break;
			}
			case 24:
			{
				// Conversion will be performed
				if (ISBITMASK(DDSFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) ||
					ISBITMASK(DDSFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
					return Render::PixelFmt_B8G8R8X8;
				break;
			}
			case 16:
			{
				if (ISBITMASK(DDSFormat, 0xf800, 0x07e0, 0x001f, 0x0000))
					return Render::PixelFmt_B5G6R5;

				break;
			}
		}
	}
	else if (DDSFormat.flags & DDS_FOURCC)
	{
		//???rename DXT to BC?

		if (DDSFormat.fourCC == MAKEFOURCC('D', 'X', 'T', '1'))
			return Render::PixelFmt_DXT1;
		if (DDSFormat.fourCC == MAKEFOURCC('D', 'X', 'T', '3'))
			return Render::PixelFmt_DXT3;
		if (DDSFormat.fourCC == MAKEFOURCC('D', 'X', 'T', '5'))
			return Render::PixelFmt_DXT5;

		// Premultiplied alpha, no separate format for now
		if (DDSFormat.fourCC == MAKEFOURCC('D', 'X', 'T', '2'))
			return Render::PixelFmt_DXT3;
		if (DDSFormat.fourCC == MAKEFOURCC('D', 'X', 'T', '4'))
			return Render::PixelFmt_DXT5;

		//// Some DDS writers write D3DFORMAT values in a FourCC field
		//switch (DDSFormat.fourCC)
		//{
		//	case 111: // D3DFMT_R16F
		//	case 114: // D3DFMT_R32F
		//	etc
		//}
	}

	return Render::PixelFmt_Invalid;
}
//---------------------------------------------------------------------

//!!!need sRGB handling!
// Duplicates CD3D11DriverFactory::DXGIFormatToPixelFormat(), because DX10+ DDS stores DXGI_FORMAT for the format value
Render::EPixelFormat DDSDX10FormatToPixelFormat(U32 DDSDX10Format)
{
	switch (DDSDX10Format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return Render::PixelFmt_R8G8B8A8;
		case DXGI_FORMAT_B8G8R8X8_UNORM:		return Render::PixelFmt_B8G8R8X8; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case DXGI_FORMAT_B8G8R8A8_UNORM:		return Render::PixelFmt_B8G8R8A8; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case DXGI_FORMAT_B5G6R5_UNORM:			return Render::PixelFmt_B5G6R5;
		case DXGI_FORMAT_BC1_UNORM:				return Render::PixelFmt_DXT1;
		case DXGI_FORMAT_BC2_UNORM:				return Render::PixelFmt_DXT3;
		case DXGI_FORMAT_BC3_UNORM:				return Render::PixelFmt_DXT5;
		//case DXGI_FORMAT_D24_UNORM_S8_UINT:		return Render::PixelFmt_D24S8;
		//case DXGI_FORMAT_D32_FLOAT:				return Render::PixelFmt_D32;
		//case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	return Render::PixelFmt_D32S8;
		case DXGI_FORMAT_UNKNOWN:
		default:								return Render::PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

const Core::CRTTI& CTextureLoaderDDS::GetResultType() const
{
	return Render::CTexture::RTTI;
}
//---------------------------------------------------------------------

bool CTextureLoaderDDS::Load(CResource& Resource)
{
	if (GPU.IsNullPtr()) FAIL;

	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;

	U64 FileSize = File->GetSize();
	if (FileSize < sizeof(DDS_HEADER) + 4) FAIL; // Too small to be a valid DDS

	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != DDS_MAGIC) FAIL;

	DDS_HEADER Header;
	if (!Reader.Read(Header) || Header.size != sizeof(DDS_HEADER) || Header.ddspf.size != sizeof(DDS_PIXELFORMAT)) FAIL;

	Render::CTextureDesc TexDesc;
	TexDesc.Width = Header.width;
	TexDesc.Height = Header.height;
	TexDesc.Depth = Header.depth;
	TexDesc.MipLevels = Clamp(Header.mipMapCount, 1U, (U32)DEM_MAX_MIP_LEVELS); //!!!set DesiredMipCount, if provided! some value for 'as in file', some for 'full chain' 
	TexDesc.MSAAQuality = Render::MSAA_None;
	bool MipDataProvided = (Header.mipMapCount > 1);

	bool IsDX10 = (Header.ddspf.flags & DDS_FOURCC) && Header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0');
	if (IsDX10)
	{
		// D3D10 and later format
		DDS_HEADER_DXT10 Header10;
		if (!Reader.Read(Header10) || Header10.arraySize == 0) FAIL;
		TexDesc.ArraySize = Header10.arraySize;

		TexDesc.Format = DDSDX10FormatToPixelFormat(Header10.dxgiFormat);
		if (TexDesc.Format == Render::PixelFmt_Invalid) FAIL;

		switch (Header10.resourceDimension)
		{
			case D3D11_RESOURCE_DIMENSION_TEXTURE1D:	TexDesc.Type = Render::Texture_1D; break;
			case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
				TexDesc.Type = (Header10.miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE) ? Render::Texture_Cube : Render::Texture_2D; break;
			case D3D11_RESOURCE_DIMENSION_TEXTURE3D:	TexDesc.Type = Render::Texture_3D; break;
			default:									FAIL;
		}
	}
	else
	{
		// Legacy format
		TexDesc.ArraySize = 1;

		TexDesc.Format = DDSFormatToPixelFormat(Header.ddspf);
		if (TexDesc.Format == Render::PixelFmt_Invalid) FAIL;

		if (Header.flags & DDS_HEADER_FLAGS_VOLUME) TexDesc.Type = Render::Texture_3D;
		else if (Header.caps2 & DDS_CUBEMAP)
		{
			// All cube faces must be defined
			if ((Header.caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES) FAIL;
			Render::Texture_Cube;
		}
		else TexDesc.Type = Render::Texture_2D;
	}

	U64 DataSize64 = FileSize - File->GetPosition();
	UPTR DataSize = (UPTR)DataSize64;
	if ((U64)DataSize != DataSize64) FAIL;

	//???!!!use mapped file instead?! at least if conversion not needed.
	void* pData = n_malloc(DataSize);
	if (File->Read(pData, DataSize) != DataSize) FAIL;

	if (!IsDX10 && Header.ddspf.RGBBitCount == 24 && TexDesc.Format == Render::PixelFmt_B8G8R8X8)
	{
		// Perform a conversion to a PixelFmt_B8G8R8X8

		const U8* pCurrData = (const U8*)pData;
		const U8* pDataEnd = (const U8*)pData + DataSize;

		DataSize = DataSize * 4 / 3;
		U8* pNewData = (U8*)n_malloc(DataSize);
		U8* pCurrNewData = pNewData;

		const bool InvertRB = ISBITMASK(Header.ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
		if (InvertRB)
		{
			while (pCurrData < pDataEnd)
			{
				*pCurrNewData = pCurrData[2];
				++pCurrNewData;
				*pCurrNewData = pCurrData[1];
				++pCurrNewData;
				*pCurrNewData = pCurrData[0];
				++pCurrNewData;
				*pCurrNewData = 0xff;
				++pCurrNewData;
				pCurrData += 3;
			}
		}
		else
		{
			while (pCurrData < pDataEnd)
			{
				*pCurrNewData = *pCurrData;
				++pCurrNewData;
				++pCurrData;
				*pCurrNewData = *pCurrData;
				++pCurrNewData;
				++pCurrData;
				*pCurrNewData = *pCurrData;
				++pCurrNewData;
				++pCurrData;
				*pCurrNewData = 0xff;
				++pCurrNewData;
			}
		}

		n_free(pData);
		pData = pNewData;
	}

	Render::PTexture Texture = GPU->CreateTexture(TexDesc, Render::Access_GPU_Read, pData, MipDataProvided);

	//???!!!use mapped file instead?! at least if conversion not needed.
	n_free(pData);

	if (Texture.IsNullPtr()) FAIL;

	Resource.Init(Texture.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}