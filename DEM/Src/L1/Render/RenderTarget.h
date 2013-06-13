#pragma once
#ifndef __DEM_L1_RENDER_TARGET_H__
#define __DEM_L1_RENDER_TARGET_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Render/RenderFwd.h>
#include <Events/EventsFwd.h>

// A surface render server renders to. Can be used as texture.
// If resolve texture and RT surface are identical, this class uses one RT texture,
// else it uses RT surface & resolve texture.

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

	// Info for recreation after device is restored
	float				W;
	float				H;
	bool				AbsoluteWH;			//???flags?
	EMSAAQuality		MSAAQuality;
	DWORD				TexW;
	DWORD				TexH;

	PTexture			RTTexture;
	IDirect3DSurface9*	pRTSurface;
	IDirect3DSurface9*	pDSSurface;

	void				GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT RTFormat, D3DFORMAT DSFormat, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality);

	DECLARE_EVENT_HANDLER(OnRenderDeviceRelease, OnDeviceRelease);
	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CRenderTarget(): RTFmt(PixelFormat_Invalid), DSFmt(PixelFormat_Invalid), pRTSurface(NULL), pDSSurface(NULL) {}

	bool				CreateDefaultRT();
	bool				Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
							   bool AbsWH, EMSAAQuality MSAA = MSAA_None, DWORD TexWidth = 0, DWORD TexHeight = 0,
							   bool UseAutoDS = false);
	void				Destroy();

	//void				Set/ClearResolveRect();
	void				Resolve();

	CTexture*			GetTexture() const { RTTexture.GetUnsafe(); }
	EPixelFormat		GetRenderTargetFormat() const { return RTFmt; }
	EPixelFormat		GetDepthStencilFormat() const { return DSFmt; }
	IDirect3DSurface9*	GetD3DRenderTargetSurface() const { return pRTSurface; }
	IDirect3DSurface9*	GetD3DDepthStencilSurface() const { return pDSSurface; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

}

#endif
