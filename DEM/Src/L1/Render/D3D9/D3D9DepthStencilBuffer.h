#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_DEPTH_STENCIL_BUFFER_H__

#include <Render/DepthStencilBuffer.h>

// D3D9 implementation of depth-stencil buffer

struct IDirect3DSurface9;

namespace Render
{

class CD3D9DepthStencilBuffer: public CDepthStencilBuffer
{
	__DeclareClass(CD3D9DepthStencilBuffer);

protected:

	IDirect3DSurface9* pDSSurface;

public:

	CD3D9DepthStencilBuffer(): pDSSurface(NULL) {}

	bool				Create(IDirect3DSurface9* pSurface); // For internal use
	virtual void		Destroy();
	IDirect3DSurface9*	GetD3DSurface() const { return pDSSurface; }
};

typedef Ptr<CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;

}

#endif
