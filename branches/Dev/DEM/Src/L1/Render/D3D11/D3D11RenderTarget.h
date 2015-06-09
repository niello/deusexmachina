#pragma once
#ifndef __DEM_L1_D3D11_RENDER_TARGET_H__
#define __DEM_L1_D3D11_RENDER_TARGET_H__

#include <Render/RenderTarget.h>

// D3D11 implementation of a render target

namespace Render
{
//typedef Ptr<class CTexture> PTexture;

class CD3D11RenderTarget: public CRenderTarget
{
protected:

public:

	CD3D11RenderTarget() {}

	bool				Create(); // For internal use
	void				Destroy();

	virtual CTexture*	GetShaderResource() const { return NULL; } //???or store PTexture inside always and avoid virtualization here?
};

typedef Ptr<CD3D11RenderTarget> PD3D11RenderTarget;

}

#endif
