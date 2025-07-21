#include <StdCfg.h>
#include "DEMTextureTarget.h"

#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>
#include <CEGUI/PropertyHelper.h>

namespace CEGUI
{
const float CDEMTextureTarget::DEFAULT_SIZE = 128.0f;
UPTR CDEMTextureTarget::s_textureNumber = 0;

CDEMTextureTarget::CDEMTextureTarget(CDEMRenderer& owner, bool addStencilBuffer, const float w, const float h)
	: CDEMRenderTarget(owner)
	, TextureTarget(addStencilBuffer)
{
	d_CEGUITexture = &static_cast<CDEMTexture&>(d_owner.createTexture(generateTextureName()));
	declareRenderSize(Sizef(w, h));
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
	if (!RT) return;
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
		d_CEGUITexture->setTexture(RT->GetShaderResource());
}
//---------------------------------------------------------------------

void CDEMTextureTarget::cleanupRenderTexture()
{
	if (d_CEGUITexture) d_CEGUITexture->setTexture(nullptr);
	RT = nullptr;
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
	d_owner.getGPUDriver()->SetRenderTarget(0, RT.Get());
	d_owner.getGPUDriver()->SetDepthStencilBuffer(nullptr);
}
//---------------------------------------------------------------------

void CDEMTextureTarget::disableRenderTexture()
{
	d_owner.getGPUDriver()->SetRenderTarget(0, PrevRT.Get());
	d_owner.getGPUDriver()->SetDepthStencilBuffer(PrevDS.Get());
	PrevRT = nullptr;
	PrevDS = nullptr;
}
//---------------------------------------------------------------------

String CDEMTextureTarget::generateTextureName()
{
	String tmp("_CEGUI_CDEMTextureTarget_");
	tmp.append(PropertyHelper<std::uint32_t>::toString(s_textureNumber++));
	return tmp;
}
//---------------------------------------------------------------------

} 
