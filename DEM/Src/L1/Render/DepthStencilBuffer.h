#pragma once
#ifndef __DEM_L1_RENDER_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_DEPTH_STENCIL_BUFFER_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>

// A video memory buffer for depth and stencil information used during rendering

namespace Render
{
struct CRenderTargetDesc;

class CDepthStencilBuffer: public Core::CObject
{
protected:

	//CRenderTargetDesc Desc;

public:

	virtual void				Destroy() = 0;
	virtual bool				IsValid() const = 0;
	const CRenderTargetDesc&	GetDesc() const { n_assert(false); return CRenderTargetDesc(); }
};

typedef Ptr<CDepthStencilBuffer> PDepthStencilBuffer;

}

#endif
