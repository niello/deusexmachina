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
	RTDesc.Width = (UPTR)d_area.getSize().d_width;
	RTDesc.Height = (UPTR)d_area.getSize().d_height;
	RTDesc.Format = Render::PixelFmt_B8G8R8A8;
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
	PrevRT = d_owner.getGPUDriver()->GetRenderTarget(0);
	PrevDS = d_owner.getGPUDriver()->GetDepthStencilBuffer();
	d_owner.getGPUDriver()->SetRenderTarget(0, RT.GetUnsafe());
	//UPTR MaxRT = d_owner.getGPUDriver()->GetMaxMultipleRenderTargetCount();
	//for (UPTR i = 1; i < MaxRT; ++i)
	//	d_owner.getGPUDriver()->SetRenderTarget(i, NULL);
	d_owner.getGPUDriver()->SetDepthStencilBuffer(NULL);
}
//---------------------------------------------------------------------

void CDEMTextureTarget::disableRenderTexture()
{
	d_owner.getGPUDriver()->SetRenderTarget(0, PrevRT.GetUnsafe());
	d_owner.getGPUDriver()->SetDepthStencilBuffer(PrevDS.GetUnsafe());
	PrevRT = NULL;
	PrevDS = NULL;
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

