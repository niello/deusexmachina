#include <StdCfg.h>
#include "DEMTexture.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Render/GPUDriver.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <Data/Buffer.h>

#include <CEGUI/System.h>
#include <CEGUI/ImageCodec.h>

namespace CEGUI
{

CDEMTexture::CDEMTexture(CDEMRenderer& Renderer, const String& name):
	Owner(Renderer),
	Size(0, 0),
	DataSize(0, 0),
	TexelScaling(0, 0),
	Name(name)
{
}
//--------------------------------------------------------------------

CDEMTexture::~CDEMTexture() {}
//--------------------------------------------------------------------

static Render::EPixelFormat CEGUIPixelFormatToPixelFormat(const Texture::PixelFormat fmt)
{
	switch (fmt)
	{
		case Texture::PixelFormat::Rgba:		return Render::PixelFmt_B8G8R8A8;
		case Texture::PixelFormat::Rgb:			return Render::PixelFmt_B8G8R8X8;
		case Texture::PixelFormat::RgbaDxt1:	return Render::PixelFmt_DXT1;
		case Texture::PixelFormat::RgbaDxt3:	return Render::PixelFmt_DXT3;
		case Texture::PixelFormat::RgbaDxt5:	return Render::PixelFmt_DXT5;
		default:								return Render::PixelFmt_Invalid;
	}
	//case Texture::PF_RGB_565:   return D3DFMT_R5G6B5;
	//case Texture::PF_RGBA_4444: return D3DFMT_A4R4G4B4;
	//return width * 2;
}
//--------------------------------------------------------------------

// Helper utility function that copies a region of a buffer containing D3DCOLOR
// values into a second buffer as RGBA values.
static void blitFromSurface(const U32* src, U32* dst, const Sizef& sz, size_t source_pitch) //size_t dest_pitch - blitRGBAToD3DCOLORSurface
{
	//!!!__mm_shuffle_epi8
	for (UPTR i = 0; i < sz.d_height; ++i)
	{
		for (UPTR j = 0; j < sz.d_width; ++j)
		{
			const U32 pixel = src[j];
			const U32 tmp = pixel & 0x00FF00FF;
			dst[j] = pixel & 0xFF00FF00 | (tmp << 16) | (tmp >> 16);
		}

		src += source_pitch / sizeof(U32);
		dst += static_cast<U32>(sz.d_width);
        //dst += dest_pitch / sizeof(U32); - blitRGBAToD3DCOLORSurface
        //src += static_cast<U32>(sz.d_width);
	}
}
//---------------------------------------------------------------------

bool CDEMTexture::isPixelFormatSupported(const PixelFormat fmt) const
{
	switch (fmt)
	{
		case PixelFormat::Rgba:
		case PixelFormat::Rgb:
		case PixelFormat::RgbaDxt1:
		case PixelFormat::RgbaDxt3:
		case PixelFormat::RgbaDxt5:	return true;
		default:					return false;
	}
}
//---------------------------------------------------------------------

void CDEMTexture::setTexture(Render::CTexture* tex)
{
	if (DEMTexture.Get() == tex) return;
	DEMTexture = tex;
	updateTextureSize();
	DataSize = Size;
	updateCachedScaleValues();
}
//---------------------------------------------------------------------

void CDEMTexture::createEmptyTexture(const Sizef& sz)
{
	Render::PTextureData Data = n_new(Render::CTextureData());
	Data->Desc.Type = Render::Texture_2D;
	Data->Desc.Width = (UPTR)sz.d_width;
	Data->Desc.Height = (UPTR)sz.d_height;
	Data->Desc.Depth = 0;
	Data->Desc.MipLevels = 1;
	Data->Desc.ArraySize = 1;
	Data->Desc.Format = Render::PixelFmt_B8G8R8A8;
	Data->Desc.MSAAQuality = Render::MSAA_None;

	//???is there any way to know will CEGUI write to texture or not?
	//can create immutable textures!
	// NB: intentionaly not a dynamic texture, CEGUI will probably write it once on load
	DEMTexture = Owner.getGPUDriver()->CreateTexture(Data, Render::Access_GPU_Read | Render::Access_GPU_Write);
	n_assert(DEMTexture.IsValidPtr());

	DataSize = sz;
	updateTextureSize();
	updateCachedScaleValues();
}
//--------------------------------------------------------------------

void CDEMTexture::loadFromFile(const String& filename, const String& resourceGroup)
{
	System* sys = System::getSingletonPtr();
	n_assert(sys);
	RawDataContainer texFile;
	sys->getResourceProvider()->loadRawDataContainer(filename, texFile, resourceGroup);
	Texture* res = sys->getImageCodec().load(texFile, this);
	sys->getResourceProvider()->unloadRawDataContainer(texFile);
	n_assert(res);
}
//--------------------------------------------------------------------

void CDEMTexture::loadFromMemory(const void* buffer, const Sizef& buffer_size, PixelFormat pixel_format)
{
	n_assert(isPixelFormatSupported(pixel_format));

	Data::PBuffer Bytes;

	// Invert to BGR(A), as DX9 doesn't support RGBA textures
	if (pixel_format == PixelFormat::Rgb)
	{
	/*	const unsigned char* src = static_cast<const unsigned char*>(buffer);
		unsigned char* dest = n_new_array(unsigned char, static_cast<unsigned int>(buffer_size.d_width * buffer_size.d_height) * 4);

		for (int i = 0; i < buffer_size.d_width * buffer_size.d_height; ++i)
		{
			dest[i * 4 + 0] = src[i * 3 + 0];
			dest[i * 4 + 1] = src[i * 3 + 1];
			dest[i * 4 + 2] = src[i * 3 + 2];
			dest[i * 4 + 3] = 0xFF;
		}*/

		const UPTR W = static_cast<UPTR>(buffer_size.d_width);
		const UPTR H = static_cast<UPTR>(buffer_size.d_height);

		Bytes.reset(n_new(Data::CBufferMallocAligned(W * H * 3, 16)));
			
		U8* pDest = static_cast<U8*>(Bytes->GetPtr());
		const U8* pSrc = static_cast<const U8*>(buffer);

		for (UPTR i = 0; i < H; ++i)
		{
			for (UPTR j = 0; j < W; ++j)
			{
				*pDest++ = pSrc[2];
				*pDest++ = pSrc[1];
				*pDest++ = pSrc[0];
				pSrc += 3;
			}
		}
	}
	else if (pixel_format == PixelFormat::Rgba)
	{
		const UPTR W = static_cast<UPTR>(buffer_size.d_width);
		const UPTR H = static_cast<UPTR>(buffer_size.d_height);

		Bytes.reset(n_new(Data::CBufferMallocAligned(W * H * 4, 16)));

		U32* pDest = static_cast<U32*>(Bytes->GetPtr());
		const U32* pSrc = static_cast<const U32*>(buffer);

		//!!!__mm_shuffle_epi8
		for (UPTR i = 0; i < H; ++i)
		{
			for (UPTR j = 0; j < W; ++j)
			{
				const U32 Pixel = *pSrc++;
				const U32 Tmp = Pixel & 0x00FF00FF;
				*pDest++ = (Pixel & 0xFF00FF00) | (Tmp << 16) | (Tmp >> 16);
			}
		}
	}
	else
	{
		// FIXME: could transfer ownership if CEGUI would allow, to keep RAM data (now destroyed after GPU texture creation)
		Bytes.reset(n_new(Data::CBufferNotOwnedImmutable(buffer)));
	}

	//!!!can reuse texture without recreation if desc is the same and not immutable!

	Render::PTextureData Data = n_new(Render::CTextureData());
	Data->Desc.Type = Render::Texture_2D;
	Data->Desc.Width = (UPTR)buffer_size.d_width;
	Data->Desc.Height = (UPTR)buffer_size.d_height;
	Data->Desc.Depth = 0;
	Data->Desc.MipLevels = 1;
	Data->Desc.ArraySize = 1;
	Data->Desc.Format = CEGUIPixelFormatToPixelFormat(pixel_format);
	Data->Desc.MSAAQuality = Render::MSAA_None;

	Data->Data = std::move(Bytes);
	Data->MipDataProvided = false;

	//???is there any way to know will CEGUI write to texture or not?
	//can create immutable textures!
	const UPTR AccessFlags = DEMTexture ? DEMTexture->GetAccess() : (Render::Access_GPU_Read | Render::Access_GPU_Write);

	DEMTexture = Owner.getGPUDriver()->CreateTexture(Data, AccessFlags);

	if (!Data->Data->IsOwning()) Data->Data.reset();

	n_assert(DEMTexture.IsValidPtr());

	DataSize = buffer_size;
	updateTextureSize();
	updateCachedScaleValues();
}
//--------------------------------------------------------------------

void CDEMTexture::blitFromMemory(const void* sourceData, const Rectf& area)
{
	if (DEMTexture.IsNullPtr() || !sourceData) return;

	Data::CBox Region(
		static_cast<int>(area.left()),
		static_cast<int>(area.top()),
		0,
		static_cast<unsigned int>(area.getWidth()),
		static_cast<unsigned int>(area.getHeight()),
		0);

	Render::CImageData SrcData;
	SrcData.RowPitch = static_cast<U32>(area.getWidth()) * 4;
	SrcData.SlicePitch = 0;

	if (DEMTexture->GetDesc().Format == Render::PixelFmt_R8G8B8A8)
	{
		SrcData.pData = (char*)sourceData;
		n_verify(Owner.getGPUDriver()->WriteToResource(*DEMTexture, SrcData, 0, 0, &Region));
	}
	else
	{
		// Convert from RGBA to texture format
		const auto PixelCount = static_cast<size_t>(area.getWidth()) * static_cast<size_t>(area.getHeight());
		U32* pBuf = (U32*)n_malloc_aligned(PixelCount * sizeof(U32), 16);
		blitFromSurface(static_cast<const U32*>(sourceData), pBuf, area.getSize(), SrcData.RowPitch);

		SrcData.pData = (char*)pBuf;
		n_verify(Owner.getGPUDriver()->WriteToResource(*DEMTexture, SrcData, 0, 0, &Region));

		n_free_aligned(pBuf);
	}
}
//--------------------------------------------------------------------

void CDEMTexture::blitToMemory(void* targetData)
{
    if (DEMTexture.IsNullPtr()) return;

	Render::CImageData Dest;
	Dest.pData = (char*)targetData;
	Dest.RowPitch = DEMTexture->GetRowPitch();
	Dest.SlicePitch = DEMTexture->GetSlicePitch();

	////!!!convert only if format is not supported!
//	blitFromSurface(static_cast<U32*>(mapped_tex.pData),
//                    static_cast<U32*>(targetData),
//                    Sizef(static_cast<float>(tex_desc.Width),
//                            static_cast<float>(tex_desc.Height)),
//                    mapped_tex.RowPitch);

	n_assert(Owner.getGPUDriver()->ReadFromResource(Dest, *DEMTexture));
}
//--------------------------------------------------------------------

void CDEMTexture::updateCachedScaleValues()
{
	const float orgW = DataSize.d_width;
	const float texW = Size.d_width;
	const float orgH = DataSize.d_height;
	const float texH = Size.d_height;

	// If texture and original data dimensions are the same, scale is based on the original size.
	// If they aren't (and source data was not stretched), scale is based on the size of the resulting texture.
	TexelScaling.x = 1.0f / ((orgW == texW) ? orgW : texW);
	TexelScaling.y = 1.0f / ((orgH == texH) ? orgH : texH);
}
//--------------------------------------------------------------------

void CDEMTexture::updateTextureSize()
{
	if (DEMTexture.Get())
	{
		const Render::CTextureDesc& Desc = DEMTexture->GetDesc();
		Size.d_width  = static_cast<float>(Desc.Width);
		Size.d_height = static_cast<float>(Desc.Height);
	}
	else Size.d_height = Size.d_width = 0.0f;
}
//--------------------------------------------------------------------

}