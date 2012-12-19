#pragma once
#ifndef __DEM_L1_RENDER_TARGET_H__
#define __DEM_L1_RENDER_TARGET_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Render/Render.h>

// A surface render server renders to. Can be used as texture.
// If resolve texture and RT surface are identical, this class uses one RT texture,
// else it uses RT surface & resolve texture.

//!!!Need also MRT! or manage MTR in render server?

//// get the actual render target surface of the texture
//hr = this->d3d9ResolveTexture->GetSurfaceLevel(0, &(this->d3d9RenderTarget));
//n_assert(SUCCEEDED(hr));

namespace Render
{
typedef Ptr<class CTexture> PTexture;

class CRenderTarget: public Core::CRefCounted
{
protected:

	bool				IsDefaultRT;		//???flags?
	bool				ResolveToTexture;	//???flags?

	D3DFORMAT			RTFmt;
	D3DFORMAT			DSFmt;

	PTexture			RTTexture;
	IDirect3DSurface9*	pRTSurface;
	IDirect3DSurface9*	pDSSurface;

	void				GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT RTFormat, D3DFORMAT DSFormat, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality);

	//!!!lost, reset!

public:

	CRenderTarget(): RTFmt(PixelFormat_Invalid), DSFmt(PixelFormat_Invalid), pRTSurface(NULL), pDSSurface(NULL) {}

	bool				CreateDefaultRT();
	bool				Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
							   bool AbsWH, EMSAAQuality MSAA = MSAA_None, DWORD TexWidth = 0, DWORD TexHeight = 0);
	void				Destroy();

	//void				Set/ClearResolveRect();
	void				Resolve();

	CTexture*			GetTexture() const { RTTexture.get_unsafe(); }
	EPixelFormat		GetRenderTargetFormat() const { return RTFmt; }
	EPixelFormat		GetDepthStencilFormat() const { return DSFmt; }
	IDirect3DSurface9*	GetD3DRenderTargetSurface() const { return pRTSurface; }
	IDirect3DSurface9*	GetD3DDepthStencilSurface() const { return pDSSurface; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

}

#endif
