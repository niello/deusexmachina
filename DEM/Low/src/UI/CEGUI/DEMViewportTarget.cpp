#include <StdCfg.h>
#include "DEMViewportTarget.h"

namespace CEGUI
{

CDEMViewportTarget::CDEMViewportTarget(CDEMRenderer& owner, const Rectf& area):
	CDEMRenderTarget(owner)
{
	setArea(area);
}
//---------------------------------------------------------------------

}
