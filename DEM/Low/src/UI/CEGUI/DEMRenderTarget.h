#pragma once
#include <CEGUI/RenderTarget.h>
#include <UI/CEGUI/DEMRenderer.h> // For covariant override of getOwner()

namespace CEGUI
{
class CDEMRenderer;

class CDEMRenderTarget: virtual public RenderTarget
{
protected:

	CDEMRenderer& d_owner;

public:

	CDEMRenderTarget(CDEMRenderer& owner);

	// implement parts of RenderTarget interface
	virtual void			activate() override;
	virtual void			unprojectPoint(const GeometryBuffer& buff, const glm::vec2& p_in, glm::vec2& p_out) const override;
	virtual CDEMRenderer&	getOwner() override { return d_owner; }
};

}
