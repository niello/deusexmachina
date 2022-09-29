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
	virtual void            updateMatrix() const override;
	virtual CDEMRenderer&	getOwner() override { return d_owner; }
};

}
