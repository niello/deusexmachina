#include <StdCfg.h>
#include "DEMTexture.h"

#include <Render/Texture.h>

#include <CEGUI/System.h>
#include <CEGUI/ImageCodec.h>

namespace CEGUI
{

static Render::EPixelFormat CEGUIFormatToPixelFormat(const Texture::PixelFormat fmt)
{
	switch (fmt)
	{
		case Texture::PF_RGBA:
		case Texture::PF_RGB:		return Render::PixelFmt_R8G8B8A8;
		case Texture::PF_RGBA_DXT1:	return Render::PixelFmt_DXT1;
		case Texture::PF_RGBA_DXT3:	return Render::PixelFmt_DXT3;
		case Texture::PF_RGBA_DXT5:	return Render::PixelFmt_DXT5;
		default:					return Render::PixelFmt_Invalid;
	}
}
//--------------------------------------------------------------------

static size_t calculateDataWidth(const size_t width, Texture::PixelFormat fmt)
{
	switch (fmt)
	{
		case Texture::PF_RGB: // also 4 because we convert to RGBA
		case Texture::PF_RGBA:		return width * 4;
		case Texture::PF_RGBA_DXT1:	return ((width + 3) / 4) * 8;
		case Texture::PF_RGBA_DXT3:
		case Texture::PF_RGBA_DXT5:	return ((width + 3) / 4) * 16;
		default:					return 0;
	}
}
//--------------------------------------------------------------------

// Helper utility function that copies an RGBA buffer into a region of a second
// buffer as D3DCOLOR data values
static void blitToSurface(const uint32* src, uint32* dst, const Sizef& sz, size_t dest_pitch)
{
	for (uint i = 0; i < sz.d_height; ++i)
	{
		for (uint j = 0; j < sz.d_width; ++j)
		{
			const uint32 pixel = src[j];
			const uint32 tmp = pixel & 0x00FF00FF;
			dst[j] = pixel & 0xFF00FF00 | (tmp << 16) | (tmp >> 16);
		}

		dst += dest_pitch / sizeof(uint32);
		src += static_cast<uint32>(sz.d_width);
	}
}
//--------------------------------------------------------------------

// Helper utility function that copies a region of a buffer containing D3DCOLOR
// values into a second buffer as RGBA values.
static void blitFromSurface(const uint32* src, uint32* dst, const Sizef& sz, size_t source_pitch)
{
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
	if (DEMTexture.GetUnsafe() != tex)
	{
		DataSize.d_width = DataSize.d_height = 0;
		DEMTexture = tex;
	}

	updateTextureSize();
	DataSize = Size;
	updateCachedScaleValues();
}
//---------------------------------------------------------------------

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

/*
void CDEMTexture::loadFromMemory(const void* buffer, const Sizef& buffer_size, PixelFormat pixel_format)
{
    n_assert(isPixelFormatSupported(pixel_format));

    DEMTexture = NULL; //???Destroy?

    const void* img_src = buffer;
    if (pixel_format == PF_RGB)
    {
        const unsigned char* src = static_cast<const unsigned char*>(buffer);
        unsigned char* dest = new unsigned char[static_cast<unsigned int>( buffer_size.d_width * buffer_size.d_height * 4 )];

        for (int i = 0; i < buffer_size.d_width * buffer_size.d_height; ++i)
        {
            dest[i * 4 + 0] = src[i * 3 + 0];
            dest[i * 4 + 1] = src[i * 3 + 1];
            dest[i * 4 + 2] = src[i * 3 + 2];
            dest[i * 4 + 3] = 0xFF;
        }

        img_src = dest;
    }

// Load texture from memory data


    if (pixel_format == PF_RGB)
        delete[] img_src;

    if (FAILED(hr))
        CEGUI_THROW(RendererException(
            "Failed to create texture from memory buffer."));

    d_dataSize = buffer_size;
    updateTextureSize();
    updateCachedScaleValues();
}
//--------------------------------------------------------------------

void CDEMTexture::blitFromMemory(const void* sourceData, const Rectf& area)
{
    if (!d_texture)
        return;

    uint32* buff = new uint32[static_cast<size_t>(area.getWidth()) *
                              static_cast<size_t>(area.getHeight())];
    blitFromSurface(static_cast<const uint32*>(sourceData), buff,
                    area.getSize(), static_cast<size_t>(area.getWidth()) * 4);

    D3D11_BOX dst_box = {static_cast<UINT>(area.left()),
                         static_cast<UINT>(area.top()),
                         0,
                         static_cast<UINT>(area.right()),
                         static_cast<UINT>(area.bottom()),
                         1};

    d_device.d_context->UpdateSubresource(d_texture, 0, &dst_box, buff,
                                          static_cast<UINT>(area.getWidth()) * 4,
                                          0);

    delete[] buff;
}
//--------------------------------------------------------------------

void CDEMTexture::blitToMemory(void* targetData)
{
    if (!d_texture)
        return;

    String exception_msg;

    D3D11_TEXTURE2D_DESC tex_desc;
    d_texture->GetDesc(&tex_desc);

    tex_desc.Usage = D3D11_USAGE_STAGING;
    tex_desc.BindFlags = 0;
    tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* offscreen;
    if (SUCCEEDED(d_device.d_device->CreateTexture2D(&tex_desc, 0, &offscreen)))
    {
        d_device.d_context->CopyResource(offscreen, d_texture);

        D3D11_MAPPED_SUBRESOURCE mapped_tex;
        if (SUCCEEDED(d_device.d_context->Map(offscreen, 0, D3D11_MAP_READ,
                                              0, &mapped_tex)))
        {
            blitFromSurface(static_cast<uint32*>(mapped_tex.pData),
                            static_cast<uint32*>(targetData),
                            Sizef(static_cast<float>(tex_desc.Width),
                                   static_cast<float>(tex_desc.Height)),
                            mapped_tex.RowPitch);

            d_device.d_context->Unmap(offscreen, 0);
        }
        else
            exception_msg.assign("ID3D11Texture2D::Map failed.");

        offscreen->Release();
    }
    else
        exception_msg.assign(
            "ID3D11Device::CreateTexture2D failed for 'offscreen'.");

    if (!exception_msg.empty())
        CEGUI_THROW(RendererException(exception_msg));
}
//--------------------------------------------------------------------

void CDEMTexture::updateCachedScaleValues()
{
    //
    // calculate what to use for x scale
    //
    const float orgW = d_dataSize.d_width;
    const float texW = d_size.d_width;

    // if texture and original data width are the same, scale is based
    // on the original size.
    // if texture is wider (and source data was not stretched), scale
    // is based on the size of the resulting texture.
    d_texelScaling.d_x = 1.0f / ((orgW == texW) ? orgW : texW);

    //
    // calculate what to use for y scale
    //
    const float orgH = d_dataSize.d_height;
    const float texH = d_size.d_height;

    // if texture and original data height are the same, scale is based
    // on the original size.
    // if texture is taller (and source data was not stretched), scale
    // is based on the size of the resulting texture.
    d_texelScaling.d_y = 1.0f / ((orgH == texH) ? orgH : texH);
}
//--------------------------------------------------------------------

void CDEMTexture::updateTextureSize()
{
    if (d_texture)
    {
        D3D11_TEXTURE2D_DESC surfDesc;
        d_texture->GetDesc(&surfDesc);
        d_size.d_width  = static_cast<float>(surfDesc.Width);
        d_size.d_height = static_cast<float>(surfDesc.Height);
    }
    else
        d_size.d_height = d_size.d_width = 0.0f;
}
//--------------------------------------------------------------------

CDEMTexture::CDEMTexture(IDevice11& device, const String& name) :
    d_device(device),
    d_texture(0),
    d_resourceView(0),
    d_size(0, 0),
    d_dataSize(0, 0),
    d_texelScaling(0, 0),
    d_name(name)
{
}
//--------------------------------------------------------------------

CDEMTexture::CDEMTexture(IDevice11& device, const String& name,
                                     const String& filename,
                                     const String& resourceGroup) :
    d_device(device),
    d_texture(0),
    d_resourceView(0),
    d_size(0, 0),
    d_dataSize(0, 0),
    d_texelScaling(0, 0),
    d_name(name)
{
    loadFromFile(filename, resourceGroup);
}

//----------------------------------------------------------------------------//
Direct3D11Texture::Direct3D11Texture(IDevice11& device, const String& name,
                                     const Sizef& sz) :
    d_device(device),
    d_texture(0),
    d_resourceView(0),
    d_size(0, 0),
    d_dataSize(0, 0),
    d_texelScaling(0, 0),
    d_name(name)
{
    D3D11_TEXTURE2D_DESC tex_desc;
    ZeroMemory(&tex_desc, sizeof(tex_desc));
    tex_desc.Width = static_cast<UINT>(sz.d_width);
    tex_desc.Height = static_cast<UINT>(sz.d_height);
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;
    tex_desc.MipLevels = 1;

    if (FAILED(d_device.d_device->CreateTexture2D(&tex_desc, 0, &d_texture)))
        CEGUI_THROW(RendererException(
            "Failed to create texture with specified size."));

    DataSize = sz;
    updateTextureSize();
    updateCachedScaleValues();
}

//----------------------------------------------------------------------------//
Direct3D11Texture::Direct3D11Texture(IDevice11& device, const String& name,
                                     ID3D11Texture2D* tex) :
    d_device(device),
    d_texture(0),
    d_resourceView(0),
    d_size(0, 0),
    d_dataSize(0, 0),
    d_texelScaling(0, 0),
    d_name(name)
{
    setDirect3DTexture(tex);
}
*/
}