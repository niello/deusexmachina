#include <StdCfg.h>
#include "DEMViewportTarget.h"

#include <Render/GPUDriver.h>
#include <UI/CEGUI/DEMRenderer.h>

namespace CEGUI
{

CDEMViewportTarget::CDEMViewportTarget(CDEMRenderer& owner): CDEMRenderTarget<>(owner)
{
	Render::CViewport VP;
	n_assert(d_owner.getGPUDriver()->GetViewport(0, VP));

	Rectf area(
		Vector2f(static_cast<float>(VP.Left), static_cast<float>(VP.Top)),
		Sizef(static_cast<float>(VP.Width), static_cast<float>(VP.Height)));

	setArea(area);
}
//--------------------------------------------------------------------

}

// Implementation of template base class
#include "DEMRenderTarget.inl"

