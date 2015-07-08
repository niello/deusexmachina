#include <StdCfg.h>
#include "DEMTexture.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Render/GPUDriver.h>
#include <Render/Texture.h>

#include <CEGUI/System.h>
#include <CEGUI/ImageCodec.h>

namespace CEGUI
{

static Render::EPixelFormat CEGUIPixelFormatToPixelFormat(const Texture::PixelFormat fmt)
{
	switch (fmt)
	{
		case Texture::PF_RGBA:
		case Texture::PF_RGB:		return Render::PixelFmt_R8G8B8A8; //???need inverse format to avoid blitFromSurface?
		case Texture::PF_RGBA_DXT1:	return Render::PixelFmt_DXT1;
		case Texture::PF_RGBA_DXT3:	return Render::PixelFmt_DXT3;
		case Texture::PF_RGBA_DXT5:	return Render::PixelFmt_DXT5;
		default:					return Render::PixelFmt_Invalid;
	}
}
//--------------------------------------------------------------------

// Helper utility function that copies a region of a buffer containing D3DCOLOR
// values into a second buffer as RGBA values.
static void blitFromSurface(const uint32* src, uint32* dst, const Sizef& sz, size_t source_pitch) //size_t dest_pitch - blitRGBAToD3DCOLORSurface
{
	//!!!__mm_shuffle_epi8
	for (uint i = 0; i < sz.d_height; ++i)
	{
		for (uint j = 0; j < sz.d_width; ++j)
		{
			const uint32 pixel = src[j];
			const uint32 tmp = pixel & 0x00FF00FF;
			dst[j] = pixel & 0xFF00FF00 | (tmp << 16) | (tmp >> 16);
		}

		src += source_pitch / sizeof(uint32);
		dst += static_cast<uint32>(sz.d_width);
        //dst += dest_pitch / sizeof(uint32); - blitRGBAToD3DCOLORSurface
        //src += static_cast<uint32>(sz.d_width);
	}
}
//---------------------------------------------------------------------

bool CDEMTexture::isPixelFormatSupported(const PixelFormat fmt) const
{
	switch (fmt)
	{
		case PF_RGBA:
		case PF_RGB:
		case PF_RGBA_DXT1:
		case PF_RGBA_DXT3:
		case PF_RGBA_DXT5:	return true;
		default:			return false;
	}
}
//---------------------------------------------------------------------

void CDEMTexture::setTexture(Render::CTexture* tex)
{
	if (DEMTexture.GetUnsafe() == tex) return;
	DEMTexture = tex;
	updateTextureSize();
	DataSize = Size;
	updateCachedScaleValues();
}
//---------------------------------------------------------------------

void CDEMTexture::createEmptyTexture(const Sizef& sz)
{
	Render::CTextureDesc Desc;
	Desc.Type = Render::Texture_2D;
	Desc.Width = (DWORD)sz.d_width;
	Desc.Height = (DWORD)sz.d_height;
	Desc.Depth = 0;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = Render::PixelFmt_R8G8B8A8;
	Desc.MSAAQuality = Render::MSAA_None;

	DEMTexture = Owner.getGPUDriver()->CreateTexture(Desc, Render::Access_GPU_Read | Render::Access_GPU_Write);
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

	const void* img_src = buffer;
	if (pixel_format == PF_RGB)
	{
		const unsigned char* src = static_cast<const unsigned char*>(buffer);
		unsigned char* dest = n_new_array(unsigned char, static_cast<unsigned int>(buffer_size.d_width * buffer_size.d_height) * 4);

		for (int i = 0; i < buffer_size.d_width * buffer_size.d_height; ++i)
		{
			dest[i * 4 + 0] = src[i * 3 + 0];
			dest[i * 4 + 1] = src[i * 3 + 1];
			dest[i * 4 + 2] = src[i * 3 + 2];
			dest[i * 4 + 3] = 0xFF;
		}

		img_src = dest;
	}

	//!!!can reuse texture without recreation if desc is the same and not immutable!

	Render::CTextureDesc Desc;
	Desc.Type = Render::Texture_2D;
	Desc.Width = (DWORD)buffer_size.d_width;
	Desc.Height = (DWORD)buffer_size.d_height;
	Desc.Depth = 0;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = CEGUIPixelFormatToPixelFormat(pixel_format);
	Desc.MSAAQuality = Render::MSAA_None;

	//???is there any way to know will CEGUI write to texture or not?
	//can create immutable textures!
	DEMTexture = Owner.getGPUDriver()->CreateTexture(Desc, Render::Access_GPU_Read | Render::Access_GPU_Write, img_src);

	if (pixel_format == PF_RGB) n_delete_array(img_src);

	n_assert(DEMTexture.IsValidPtr());

	DataSize = buffer_size;
	updateTextureSize();
	updateCachedScaleValues();
}
//--------------------------------------------------------------------

void CDEMTexture::blitFromMemory(const void* sourceData, const Rectf& area)
{
	if (DEMTexture.IsNullPtr()) return;

	UINT SrcPitch = ((UINT)area.getWidth()) * 4;

	//!!!convert only if format is not supported!
	uint32* pBuf = n_new_array(uint32, static_cast<size_t>(area.getWidth()) * static_cast<size_t>(area.getHeight()));
	blitFromSurface(static_cast<const uint32*>(sourceData), pBuf, area.getSize(), SrcPitch);

	Render::CImageData SrcData;
	SrcData.pData = (char*)pBuf;
	SrcData.RowPitch = SrcPitch;

	Data::CBox Region(
		static_cast<int>(area.left()),
		static_cast<int>(area.top()),
		0,
		static_cast<unsigned int>(area.getWidth()),
		static_cast<unsigned int>(area.getHeight()),
		0);

	bool Result = Owner.getGPUDriver()->WriteToResource(*DEMTexture, SrcData, 0, 0, &Region);

	n_delete_array(pBuf);
	n_assert(Result);
}
//--------------------------------------------------------------------

void CDEMTexture::blitToMemory(void* targetData)
{
    if (DEMTexture.IsNullPtr()) return;

	Render::CImageData Dest;
	Dest.pData = (char*)targetData;
	Dest.RowPitch = DEMTexture->GetRowPitch();

////!!!convert only if format is not supported!
//	blitFromSurface(static_cast<uint32*>(mapped_tex.pData),
//                    static_cast<uint32*>(targetData),
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
	TexelScaling.d_x = 1.0f / ((orgW == texW) ? orgW : texW);
	TexelScaling.d_y = 1.0f / ((orgH == texH) ? orgH : texH);
}
//--------------------------------------------------------------------

void CDEMTexture::updateTextureSize()
{
	if (DEMTexture)
	{
		const Render::CTextureDesc& Desc = DEMTexture->GetDesc();
		Size.d_width  = static_cast<float>(Desc.Width);
		Size.d_height = static_cast<float>(Desc.Height);
	}
	else Size.d_height = Size.d_width = 0.0f;
}
//--------------------------------------------------------------------

}