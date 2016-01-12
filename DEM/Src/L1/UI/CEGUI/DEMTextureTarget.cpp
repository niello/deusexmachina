#include <StdCfg.h>
#include "DEMTextureTarget.h"

#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>

namespace CEGUI
{
const float CDEMTextureTarget::DEFAULT_SIZE = 128.0f;
uint CDEMTextureTarget::s_textureNumber = 0;

CDEMTextureTarget::CDEMTextureTarget(CDEMRenderer& owner, const float size): CDEMRenderTarget<TextureTarget>(owner)
{
	d_CEGUITexture = &static_cast<CDEMTexture&>(d_owner.createTexture(generateTextureName()));
	declareRenderSize(Sizef(size, size));
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

void CDEMTextureTarget::clear()
{
	if (RT.IsNullPtr()) return;
	d_owner.getGPUDriver()->ClearRenderTarget(*RT, vector4::Zero);
}
//---------------------------------------------------------------------

Texture& CDEMTextureTarget::getTexture() const
{
	return *d_CEGUITexture;
}
//---------------------------------------------------------------------

void CDEMTextureTarget::declareRenderSize(const Sizef& sz)
{
	if ((d_area.getWidth() >= sz.d_width) && (d_area.getHeight() >= sz.d_height)) return;
	setArea(Rectf(d_area.getPosition(), sz));
	resizeRenderTexture();
	clear();
}
//---------------------------------------------------------------------

void CDEMTextureTarget::initialiseRenderTexture()
{
	Render::CRenderTargetDesc RTDesc;
	RTDesc.Width = (DWORD)d_area.getSize().d_width;
	RTDesc.Height = (DWORD)d_area.getSize().d_height;
	RTDesc.Format = Render::PixelFmt_R8G8B8A8;
	RTDesc.MSAAQuality = Render::MSAA_None;
	RTDesc.UseAsShaderInput = true;
	RTDesc.MipLevels = 1; // Can test 0 = full mipmap chain

	RT = d_owner.getGPUDriver()->CreateRenderTarget(RTDesc);
	n_assert(RT.IsValidPtr());

	if (d_CEGUITexture)
	{
		d_CEGUITexture->setTexture(RT->GetShaderResource());
		d_CEGUITexture->setOriginalDataSize(d_area.getSize());
	}
}
//---------------------------------------------------------------------

void CDEMTextureTarget::cleanupRenderTexture()
{
	if (d_CEGUITexture) d_CEGUITexture->setTexture(NULL);
	RT = NULL;
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
	//d_device.d_context->OMGetRenderTargets(1, &d_previousRenderTargetView, &d_previousDepthStencilView);
	d_owner.getGPUDriver()->SetRenderTarget(0, RT.GetUnsafe());
	DWORD MaxRT = d_owner.getGPUDriver()->GetMaxMultipleRenderTargetCount();
	for (DWORD i = 1; i < MaxRT; ++i)
		d_owner.getGPUDriver()->SetRenderTarget(i, NULL);
	d_owner.getGPUDriver()->SetDepthStencilBuffer(NULL);
}
//---------------------------------------------------------------------

void CDEMTextureTarget::disableRenderTexture()
{
	//if (d_previousRenderTargetView) d_previousRenderTargetView->Release();
	//if (d_previousDepthStencilView) d_previousDepthStencilView->Release();

	//d_device.d_context->OMSetRenderTargets(1, &d_previousRenderTargetView, d_previousDepthStencilView);

	//d_previousRenderTargetView = 0;
	//d_previousDepthStencilView = 0;

	//OR
	//d_owner.getGPUDriver()->SetRenderTarget(0, NULL); // with defaulting to implicit SC backbuffer for D3D9
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

} 

// Implementation of template base class
#include "DEMRenderTarget.inl"

