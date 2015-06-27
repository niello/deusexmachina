#include <StdCfg.h>
#include "DEMViewportTarget.h"

namespace CEGUI
{
/*
CDEMViewportTarget::CDEMViewportTarget(CDEMRenderer& owner): CDEMRenderTarget<>(owner)
{
	D3D11_VIEWPORT vp;
	UINT vp_count = 1;
	d_device.d_context->RSGetViewports(&vp_count, &vp);
	if (vp_count != 1)
		CEGUI_THROW(RendererException(
		"Unable to access required view port information from "
		"ID3D11Device."));

	Rectf area(
		Vector2f(static_cast<float>(vp.TopLeftX), static_cast<float>(vp.TopLeftY)),
		Sizef(static_cast<float>(vp.Width), static_cast<float>(vp.Height)));

	setArea(area);
}
//--------------------------------------------------------------------
*/
}

// Implementation of template base class
#include "DEMRenderTarget.inl"

