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
public:

	enum EClearFlag
	{
		Clear_Color		= 0x01,
		Clear_Depth		= 0x02,
		Clear_Stencil	= 0x04
	};

protected:

	bool				IsDefaultRT;		//???flags?
	bool				ResolveToTexture;	//???flags?
	DWORD				ClearFlags;			//???to flags?
	vector4				ClearColor;
	float				ClearDepth;
	uchar				ClearStencil;

	PTexture			RTTexture;
	IDirect3DSurface9*	pRTSurface;
	IDirect3DSurface9*	pDSSurface;

	void				GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT RTFormat, D3DFORMAT DSFormat, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality);

	//!!!lost, reset!

public:

	CRenderTarget(): ClearFlags(0), ClearDepth(1.f), ClearStencil(0), pRTSurface(NULL), pDSSurface(NULL) {}

	bool				CreateDefaultRT();
	bool				Create(CStrID TextureID, D3DFORMAT RTFormat, D3DFORMAT DSFormat, float Width, float Height,
							   bool AbsWH, EMSAAQuality MSAA = MSAA_None, DWORD TexWidth = 0, DWORD TexHeight = 0);
	void				Destroy();

	void				SetClearParams(DWORD Flags, const vector4& Color, float Depth, uchar Stencil);
	//void				Set/ClearResolveRect();

	CTexture*			GetTexture() const { RTTexture.get_unsafe(); }
	IDirect3DSurface9*	GetD3DRenderTargetSurface() const { return pRTSurface; }
	IDirect3DSurface9*	GetD3DDepthStencilSurface() const { return pDSSurface; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

inline void CRenderTarget::SetClearParams(DWORD Flags, const vector4& Color, float Depth, uchar Stencil)
{
	ClearFlags = Flags;
	if (ClearFlags & Clear_Color) ClearColor = Color;
	if (ClearFlags & Clear_Depth) ClearDepth = Depth;
	if (ClearFlags & Clear_Stencil) ClearStencil = Stencil;
}
//---------------------------------------------------------------------

}

#endif
