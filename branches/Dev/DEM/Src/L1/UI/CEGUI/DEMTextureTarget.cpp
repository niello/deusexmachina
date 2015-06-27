#include <StdCfg.h>
#include "DEMTextureTarget.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>
//#include "CEGUI/PropertyHelper.h"

namespace CEGUI
{
const float CDEMTextureTarget::DEFAULT_SIZE = 128.0f;
uint CDEMTextureTarget::s_textureNumber = 0;

CDEMTextureTarget::CDEMTextureTarget(CDEMRenderer& owner): CDEMRenderTarget<TextureTarget>(owner)
{
	// this essentially creates a 'null' CEGUI::Texture
	d_CEGUITexture = &static_cast<CDEMTexture&>(d_owner.createTexture(generateTextureName()));

	// setup area and cause the initial texture to be generated.
	declareRenderSize(Sizef(DEFAULT_SIZE, DEFAULT_SIZE));
}
//---------------------------------------------------------------------

CDEMTextureTarget::~CDEMTextureTarget()
{
	cleanupRenderTexture();
	d_owner.destroyTexture(*d_CEGUITexture);
}
//---------------------------------------------------------------------

void CDEMTextureTarget::activate()
{
	enableRenderTexture();
	CDEMRenderTarget::activate();
}
//---------------------------------------------------------------------

void CDEMTextureTarget::deactivate()
{
	CDEMRenderTarget::deactivate();
	disableRenderTexture();
}
//---------------------------------------------------------------------

/*
void CDEMTextureTarget::clear()
{
	//!!!D3D9 must bind texture, then clear, then unbind! implement in GPUDrv

	const float colour[] = { 0, 0, 0, 0 };
	d_device.d_context->ClearRenderTargetView(d_renderTargetView, colour);
}
//---------------------------------------------------------------------

Texture& CDEMTextureTarget::getTexture() const
{
	return *d_CEGUITexture;
}
//---------------------------------------------------------------------

void CDEMTextureTarget::declareRenderSize(const Sizef& sz)
{
	// exit if current size is enough
	if ((d_area.getWidth() >= sz.d_width) && (d_area.getHeight() >=sz.d_height)) return;

	setArea(Rectf(d_area.getPosition(), sz));
	resizeRenderTexture();
	clear();
}
//---------------------------------------------------------------------

void CDEMTextureTarget::initialiseRenderTexture()
{
    // Create the render target texture
    D3D11_TEXTURE2D_DESC tex_desc;
    ZeroMemory(&tex_desc, sizeof(tex_desc));
    tex_desc.Width = static_cast<UINT>(d_area.getSize().d_width);
    tex_desc.Height = static_cast<UINT>(d_area.getSize().d_height);
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    d_device.d_device->CreateTexture2D(&tex_desc, 0, &d_texture);

    // create render target view, so we can render to the thing
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    rtv_desc.Format = tex_desc.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;
    d_device.d_device->CreateRenderTargetView(d_texture, &rtv_desc, &d_renderTargetView);

    d_CEGUITexture->setDirect3DTexture(d_texture);
    d_CEGUITexture->setOriginalDataSize(d_area.getSize());
}
//---------------------------------------------------------------------

void CDEMTextureTarget::cleanupRenderTexture()
{
    if (d_renderTargetView)
    {
        d_renderTargetView->Release();
        d_renderTargetView = 0;
    }
    if (d_texture)
    {
        d_CEGUITexture->setDirect3DTexture(0);
        d_texture->Release();
        d_texture = 0;
    }
}
//---------------------------------------------------------------------

void CDEMTextureTarget::resizeRenderTexture()
{
	cleanupRenderTexture();
	initialiseRenderTexture();
}
//---------------------------------------------------------------------

void CDEMTextureTarget::enableRenderTexture()
{
	//???don't store prev?
	d_device.d_context->OMGetRenderTargets(1, &d_previousRenderTargetView, &d_previousDepthStencilView);
	d_device.d_context->OMSetRenderTargets(1, &d_renderTargetView, 0);
}
//---------------------------------------------------------------------

void CDEMTextureTarget::disableRenderTexture()
{
    if (d_previousRenderTargetView) d_previousRenderTargetView->Release();
    if (d_previousDepthStencilView) d_previousDepthStencilView->Release();

    d_device.d_context->OMSetRenderTargets(1, &d_previousRenderTargetView,
                                d_previousDepthStencilView);

    d_previousRenderTargetView = 0;
    d_previousDepthStencilView = 0;
}
//---------------------------------------------------------------------

String CDEMTextureTarget::generateTextureName()
{
	char StrIdx[12];
	_itoa_s(s_textureNumber++, StrIdx, 10);
	String tmp("CEGUI_CDEMTexture_");
	tmp.append(StrIdx);
	return tmp;
}
//---------------------------------------------------------------------
*/
} 

// Implementation of template base class
#include "DEMRenderTarget.inl"

