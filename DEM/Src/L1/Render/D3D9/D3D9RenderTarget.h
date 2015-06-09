#pragma once
#ifndef __DEM_L1_D3D9_RENDER_TARGET_H__
#define __DEM_L1_D3D9_RENDER_TARGET_H__

#include <Core/Object.h>
//#include <Data/StringID.h>
#include <Render/RenderFwd.h>
//#include <Events/EventsFwd.h>

// D3D9 implementation of a render target. Note that an MSAA target must be created as a
// render target surface whereas a non-MSAA target can be created from a texture directly.

struct IDirect3DSurface9;
typedef enum _D3DFORMAT D3DFORMAT;

namespace Render
{
typedef Ptr<class CTexture> PTexture;

class CRenderTarget: public Core::CObject
{
protected:

	PTexture			RTTexture;
	IDirect3DSurface9*	pRTSurface;
	D3DFORMAT			D3DFormat;

	//DECLARE_EVENT_HANDLER(OnRenderDeviceRelease, OnDeviceRelease);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CRenderTarget(): pRTSurface(NULL) {}

	// Init with API-specific pointers

	//bool				CreateDefaultRT();
	//bool				Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
	//						   bool AbsWH, EMSAAQuality MSAA = MSAA_None, DWORD TexWidth = 0, DWORD TexHeight = 0,
	//						   bool UseAutoDS = false);
	//void				Destroy();

	////void				Set/ClearResolveRect();
	//void				Resolve();

	//CTexture*			GetTexture() const { RTTexture.GetUnsafe(); }
	IDirect3DSurface9*	GetD3DSurface() const { return pRTSurface; }
	D3DFORMAT			GetD3DFormat() const { return D3DFormat; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

}

#endif
