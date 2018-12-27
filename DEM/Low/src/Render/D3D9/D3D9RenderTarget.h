#pragma once
#ifndef __DEM_L1_D3D9_RENDER_TARGET_H__
#define __DEM_L1_D3D9_RENDER_TARGET_H__

#include <Render/RenderTarget.h>

// D3D9 implementation of a render target. Note that an MSAA target must be created as a
// render target surface whereas a non-MSAA target can be created from a texture directly.

struct IDirect3DSurface9;
typedef enum _D3DFORMAT D3DFORMAT;

namespace Render
{
typedef Ptr<class CD3D9Texture> PD3D9Texture;

class CD3D9RenderTarget: public CRenderTarget
{
	__DeclareClass(CD3D9RenderTarget);

protected:

	IDirect3DSurface9*	pRTSurface;
	PD3D9Texture		SRTexture;
	bool				NeedResolve; // Autoresolve RT surface to SR texture

	void				InternalDestroy();

	//DECLARE_EVENT_HANDLER(OnRenderDeviceRelease, OnDeviceRelease);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CD3D9RenderTarget(): pRTSurface(NULL) {}
	virtual ~CD3D9RenderTarget() { InternalDestroy(); }

	bool				Create(IDirect3DSurface9* pSurface, PD3D9Texture Texture); // For internal use
	virtual void		Destroy() { InternalDestroy(); }
	virtual bool		IsValid() const { return !!pRTSurface; }
	virtual bool		CopyResolveToTexture(PTexture Dest /*, region*/) const;
	virtual CTexture*	GetShaderResource() const;
	IDirect3DSurface9*	GetD3DSurface() const { return pRTSurface; }
};

typedef Ptr<CD3D9RenderTarget> PD3D9RenderTarget;

}

#endif
