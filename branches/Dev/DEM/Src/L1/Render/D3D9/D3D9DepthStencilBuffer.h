#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_DEPTH_STENCIL_BUFFER_H__

#include <Render/DepthStencilBuffer.h>

// D3D9 implementation of a depth-stencil buffer.
// Reading as a texture is currently unsupported though it can be added through INTZ.

struct IDirect3DSurface9;

namespace Render
{

class CD3D9DepthStencilBuffer: public CDepthStencilBuffer
{
	__DeclareClass(CD3D9DepthStencilBuffer);

protected:

	IDirect3DSurface9* pDSSurface;

	void				InternalDestroy();

public:

	CD3D9DepthStencilBuffer(): pDSSurface(NULL) {}
	virtual ~CD3D9DepthStencilBuffer() { InternalDestroy(); }

	bool				Create(IDirect3DSurface9* pSurface); // For internal use
	virtual void		Destroy() { InternalDestroy(); }
	virtual bool		IsValid() const { return !!pDSSurface; }
	virtual CTexture*	GetShaderResource() const { return NULL; }
	IDirect3DSurface9*	GetD3DSurface() const { return pDSSurface; }
};

typedef Ptr<CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;

}

#endif
