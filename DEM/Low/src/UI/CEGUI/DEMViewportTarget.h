#pragma once
#include <UI/CEGUI/DEMRenderTarget.h>
#include <CEGUI/RenderTarget.h>

namespace CEGUI
{

class CDEMViewportTarget: public CDEMRenderTarget
{
public:

	CDEMViewportTarget(CDEMRenderer& owner, const Rectf& area);

	virtual bool isImageryCache() const { return false; }
};

}
