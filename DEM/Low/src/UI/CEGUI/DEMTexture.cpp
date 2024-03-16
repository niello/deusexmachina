#include <StdCfg.h>
#include "DEMTexture.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Render/GPUDriver.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <Data/Buffer.h>

#include <CEGUI/System.h>
#include <CEGUI/ImageCodec.h>
#include <CEGUI/Rectf.h>
#include <CEGUI/ResourceProvider.h>
#include <CEGUI/DataContainer.h>

namespace CEGUI
{

CDEMTexture::CDEMTexture(CDEMRenderer& Renderer, const String& Name):
	_Owner(Renderer),
	_Size(0.f, 0.f),
	_TexelScaling(0.f, 0.f),
	_Name(Name)
{
}
//--------------------------------------------------------------------

CDEMTexture::~CDEMTexture() = default;
//--------------------------------------------------------------------

static Render::EPixelFormat CEGUIPixelFormatToPixelFormat(const Texture::PixelFormat fmt)
{
	switch (fmt)
	{
		case Texture::PixelFormat::Rgba:     return Render::PixelFmt_B8G8R8A8;
		case Texture::PixelFormat::Rgb:      return Render::PixelFmt_B8G8R8X8; // NB: expand to 32 bpp
		case Texture::PixelFormat::RgbDxt1:  return Render::PixelFmt_DXT1; // NB: OpenGL has different RGB & RGBA DXT1
		case Texture::PixelFormat::RgbaDxt1: return Render::PixelFmt_DXT1;
		case Texture::PixelFormat::RgbaDxt3: return Render::PixelFmt_DXT3;
		case Texture::PixelFormat::RgbaDxt5: return Render::PixelFmt_DXT5;
		default:                             return Render::PixelFmt_Invalid;
	}
/* TODO:
		Rgba4444,
		Rgb565,
		Pvrtc2,
		Pvrtc4,
	};
*/
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
		case PixelFormat::RgbDxt1:
		case PixelFormat::RgbaDxt1:
		case PixelFormat::RgbaDxt3:
		case PixelFormat::RgbaDxt5:	return true;
	}

	return false;
}
//---------------------------------------------------------------------

void CDEMTexture::setTexture(Render::CTexture* pTexture)
{
	if (_DEMTexture == pTexture) return;
	_DEMTexture = pTexture;
	updateTextureSize();
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
	_DEMTexture = _Owner.getGPUDriver()->CreateTexture(Data, Render::Access_GPU_Read | Render::Access_GPU_Write);
	n_assert(_DEMTexture);

	updateTextureSize();
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

	const UPTR W = static_cast<UPTR>(buffer_size.d_width);
	const UPTR H = static_cast<UPTR>(buffer_size.d_height);

	// Invert to BGR(X/A), as DX9 doesn't support RGBA textures
	// NB: expand to 32 bpp for all renderers
	if (pixel_format == PixelFormat::Rgb)
	{
		Bytes.reset(n_new(Data::CBufferMallocAligned(W * H * 4, 16)));
			
		const U8* pSrc = static_cast<const U8*>(buffer);
		const U8* pSrcEnd = pSrc + W * H * 3;
		U8* pDest = static_cast<U8*>(Bytes->GetPtr());

		for (; pSrc < pSrcEnd; pSrc += 3)
		{
			*pDest++ = pSrc[2];
			*pDest++ = pSrc[1];
			*pDest++ = pSrc[0];
			*pDest++ = 0x00;
		}
	}
	else if (pixel_format == PixelFormat::Rgba)
	{
		Bytes.reset(n_new(Data::CBufferMallocAligned(W * H * 4, 16)));

		const U32* pSrc = static_cast<const U32*>(buffer);
		U32* pDest = static_cast<U32*>(Bytes->GetPtr());

		//!!!__mm_shuffle_epi8
		for (UPTR i = 0; i < W * H; ++i)
		{
			const U32 Pixel = *pSrc++;
			const U32 Tmp = Pixel & 0x00FF00FF;
			*pDest++ = (Pixel & 0xFF00FF00) | (Tmp << 16) | (Tmp >> 16);
		}
	}
	else
	{
		// FIXME: could transfer ownership if CEGUI would allow, to keep RAM data (now destroyed after GPU texture creation)
		Bytes.reset(n_new(Data::CBufferNotOwnedImmutable(buffer, 0)));
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
	const UPTR AccessFlags = _DEMTexture ? _DEMTexture->GetAccess() : (Render::Access_GPU_Read | Render::Access_GPU_Write);

	_DEMTexture = _Owner.getGPUDriver()->CreateTexture(Data, AccessFlags);

	if (!Data->Data->IsOwning()) Data->Data.reset();

	n_assert(_DEMTexture);

	updateTextureSize();
}
//--------------------------------------------------------------------

void CDEMTexture::blitFromMemory(const void* sourceData, const Rectf& area)
{
	if (!_DEMTexture || !sourceData) return;

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

	if (_DEMTexture->GetDesc().Format == Render::PixelFmt_R8G8B8A8)
	{
		SrcData.pData = (char*)sourceData;
		n_verify(_Owner.getGPUDriver()->WriteToResource(*_DEMTexture, SrcData, 0, 0, &Region));
	}
	else
	{
		// Convert from RGBA to texture format
		const auto PixelCount = static_cast<size_t>(area.getWidth()) * static_cast<size_t>(area.getHeight());
		U32* pBuf = (U32*)n_malloc_aligned(PixelCount * sizeof(U32), 16);
		blitFromSurface(static_cast<const U32*>(sourceData), pBuf, area.getSize(), SrcData.RowPitch);

		SrcData.pData = (char*)pBuf;
		n_verify(_Owner.getGPUDriver()->WriteToResource(*_DEMTexture, SrcData, 0, 0, &Region));

		n_free_aligned(pBuf);
	}
}
//--------------------------------------------------------------------

void CDEMTexture::blitToMemory(void* targetData)
{
    if (!_DEMTexture) return;

	Render::CImageData Dest;
	Dest.pData = (char*)targetData;
	Dest.RowPitch = _DEMTexture->GetRowPitch();
	Dest.SlicePitch = _DEMTexture->GetSlicePitch();

	////!!!convert only if format is not supported!
//	blitFromSurface(static_cast<U32*>(mapped_tex.pData),
//                    static_cast<U32*>(targetData),
//                    Sizef(static_cast<float>(tex_desc.Width),
//                            static_cast<float>(tex_desc.Height)),
//                    mapped_tex.RowPitch);

	n_verify(_Owner.getGPUDriver()->ReadFromResource(Dest, *_DEMTexture));
}
//--------------------------------------------------------------------

void CDEMTexture::updateTextureSize()
{
	if (_DEMTexture.Get())
	{
		const Render::CTextureDesc& Desc = _DEMTexture->GetDesc();
		_Size.d_width  = static_cast<float>(Desc.Width);
		_Size.d_height = static_cast<float>(Desc.Height);
		_TexelScaling.x = 1.f / _Size.d_width;
		_TexelScaling.y = 1.f / _Size.d_height;
	}
	else
	{
		_Size.d_height = 0.f;
		_Size.d_width = 0.f;
		_TexelScaling.x = 0.f;
		_TexelScaling.y = 0.f;
	}
}
//--------------------------------------------------------------------

}
