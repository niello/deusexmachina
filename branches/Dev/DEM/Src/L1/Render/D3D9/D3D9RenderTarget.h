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
typedef Ptr<class CTexture> PTexture;

class CD3D9RenderTarget: public CRenderTarget
{
protected:

	IDirect3DSurface9*	pRTSurface;
	PTexture			RTTexture;

	//DECLARE_EVENT_HANDLER(OnRenderDeviceRelease, OnDeviceRelease);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CD3D9RenderTarget(): pRTSurface(NULL) {}

	bool				Create(IDirect3DSurface9* pSurface); // For internal use
	void				Destroy();

	//bool				Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
	//						   bool AbsWH, EMSAAQuality MSAA = MSAA_None, DWORD TexWidth = 0, DWORD TexHeight = 0,
	//						   bool UseAutoDS = false);
	//void				Destroy();

	////void				Set/ClearResolveRect();
	//void				Resolve();

	virtual CTexture*	GetShaderResource() const { return RTTexture.GetUnsafe(); } //???or store PTexture inside always and avoid virtualization here?
	IDirect3DSurface9*	GetD3DSurface() const { return pRTSurface; }
};

typedef Ptr<CD3D9RenderTarget> PD3D9RenderTarget;

}

#endif
