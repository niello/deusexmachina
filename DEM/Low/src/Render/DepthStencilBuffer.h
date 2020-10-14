#pragma once
#ifndef __DEM_L1_RENDER_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_DEPTH_STENCIL_BUFFER_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>

// A video memory buffer for depth and stencil information.

namespace Render
{

class CDepthStencilBuffer: public Core::CObject
{
	RTTI_CLASS_DECL(Render::CDepthStencilBuffer, Core::CObject);

protected:

	CRenderTargetDesc Desc;

public:

	virtual void				Destroy() = 0;
	virtual bool				IsValid() const = 0;
	virtual CTexture*			GetShaderResource() const = 0;
	const CRenderTargetDesc&	GetDesc() const { return Desc; }
};

typedef Ptr<CDepthStencilBuffer> PDepthStencilBuffer;

}

#endif
