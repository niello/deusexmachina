#include <StdCfg.h>
#include "DEMViewportTarget.h"

#include <Render/GPUDriver.h>
#include <UI/CEGUI/DEMRenderer.h>

namespace CEGUI
{

// VC++ 2013 doesn't compile any symbols in this module if this is placed in a header
CDEMViewportTarget::CDEMViewportTarget(CDEMRenderer& owner, const Rectf& area):
	CDEMRenderTarget<RenderTarget>(owner)
{
	setArea(area);
}
//---------------------------------------------------------------------

}

// Implementation of template base class
#include "DEMRenderTarget.inl"

