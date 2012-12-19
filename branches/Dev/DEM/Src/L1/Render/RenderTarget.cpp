#include "RenderTarget.h"

#include <Render/RenderServer.h>

namespace Render
{

//???bool Windowed param?
inline void CRenderTarget::GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT RTFormat, D3DFORMAT DSFormat,
											D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality)
{
#if DEM_D3D_DEBUG
	OutType = D3DMULTISAMPLE_NONE;
	OutQuality = 0;
#else
	switch (MSAA)
	{
		case MSAA_None:	OutType = D3DMULTISAMPLE_NONE; break;
		case MSAA_2x:	OutType = D3DMULTISAMPLE_2_SAMPLES; break;
		case MSAA_4x:	OutType = D3DMULTISAMPLE_4_SAMPLES; break;
		case MSAA_8x:	OutType = D3DMULTISAMPLE_8_SAMPLES; break;
	};

	DWORD QualLevels = 0;
	HRESULT hr = RenderSrv->GetD3D()->CheckDeviceMultiSampleType(	RenderSrv->GetD3DAdapter(),
																	DEM_D3D_DEVICETYPE,
																	RTFormat,
																	FALSE,
																	OutType,
																	&QualLevels);
	if (hr == D3DERR_NOTAVAILABLE)
	{
		OutType = D3DMULTISAMPLE_NONE;
		OutQuality = 0;
		return;
	}
	n_assert(SUCCEEDED(hr));

	OutQuality = QualLevels ? QualLevels - 1 : 0;

	if (DSFormat == D3DFMT_UNKNOWN) return;

	hr = RenderSrv->GetD3D()->CheckDeviceMultiSampleType(	RenderSrv->GetD3DAdapter(),
															DEM_D3D_DEVICETYPE,
															DSFormat,
															FALSE,
															OutType,
															NULL);
	if (hr == D3DERR_NOTAVAILABLE)
	{
		OutType = D3DMULTISAMPLE_NONE;
		OutQuality = 0;
		return;
	}
	n_assert(SUCCEEDED(hr));
#endif
}
//---------------------------------------------------------------------

bool CRenderTarget::CreateDefaultRT()
{
	n_assert(!RTTexture.isvalid());
	//???assert not created? empty texture is not enough!

	IsDefaultRT = true;

	const CDisplayMode& DispMode = RenderSrv->GetDisplay().GetDisplayMode();
	//float Width = DispMode.Width;
	//float Height = DispMode.Height;
	//EMSAAQuality MSAA = RenderSrv->GetDisplay().AntiAliasQuality;
	RTFmt = DispMode.PixelFormat;

	pRTSurface = NULL;
	n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRTSurface)));
	pDSSurface = NULL;
	n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->GetDepthStencilSurface(&pDSSurface)));

	if (pDSSurface)
	{
		D3DSURFACE_DESC Desc;
		memset(&Desc, 0, sizeof(Desc));
		n_assert(SUCCEEDED(pDSSurface->GetDesc(&Desc)));
		DSFmt = Desc.Format;
	}
	else DSFmt = PixelFormat_Invalid;

	OK;
}
//---------------------------------------------------------------------

bool CRenderTarget::Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
						   bool AbsWH, EMSAAQuality MSAA, DWORD TexWidth, DWORD TexHeight
						   /*, use mips, shared DS*/)
{
	if (!TextureID.IsValid() || RTFormat == D3DFMT_UNKNOWN) FAIL;

	//???assert not created?

	IsDefaultRT = false;

	//???members?
	DWORD AbsWidth;
	DWORD AbsHeight;

	if (AbsWH)
	{
		AbsWidth = (DWORD)Width;
		AbsHeight = (DWORD)Height;
	}
	else
	{
		const CDisplayMode& DispMode = RenderSrv->GetDisplay().GetDisplayMode();
		AbsWidth = (DWORD)(Width * (float)DispMode.Width);
		AbsHeight = (DWORD)(Height * (float)DispMode.Height);
	}

	RTTexture = RenderSrv->TextureMgr.GetTypedResource(TextureID);
	n_assert2(!RTTexture->IsLoaded(), "Render target specifies TextureID of already loaded texture");

	D3DMULTISAMPLE_TYPE D3DMSAAType;
	DWORD D3DMSAAQuality;
	GetD3DMSAAParams(MSAA, RTFormat, DSFormat, D3DMSAAType, D3DMSAAQuality);

	ResolveToTexture = (D3DMSAAType != D3DMULTISAMPLE_NONE) ||
		(TexWidth && TexWidth != AbsWidth) ||
		(TexHeight && TexHeight != AbsHeight); // || use mips

	TexWidth = TexWidth ? TexWidth : AbsWidth;
	TexHeight = TexHeight ? TexHeight : AbsHeight;

	if (ResolveToTexture)
	{
		HRESULT hr = RenderSrv->GetD3DDevice()->CreateRenderTarget(	AbsWidth, AbsHeight, RTFormat,
																	D3DMSAAType, D3DMSAAQuality,
																	FALSE, &pRTSurface, NULL);
		n_assert(SUCCEEDED(hr));
        //if (mipMapsEnabled) usage |= D3DUSAGE_AUTOGENMIPMAP; //???is applicable to 1-level RT texture? N3 code.
	}

	n_assert(RTTexture->CreateRenderTarget(RTFormat, TexWidth, TexHeight));

	if (!ResolveToTexture)
		n_assert(SUCCEEDED(RTTexture->GetD3D9Texture()->GetSurfaceLevel(0, &pRTSurface)));

	RTFmt = RTFormat;

	if (DSFormat != D3DFMT_UNKNOWN)
	{
		HRESULT hr = RenderSrv->GetD3DDevice()->CreateDepthStencilSurface(AbsWidth, AbsHeight, DSFormat,
																		  D3DMSAAType, D3DMSAAQuality, TRUE,
																		  &pDSSurface, NULL);
		n_assert(SUCCEEDED(hr));
	}

	DSFmt = DSFormat;

	OK;
}
//---------------------------------------------------------------------

void CRenderTarget::Destroy()
{
	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface); //???what if shared? may AddRef in Create
	if (RTTexture.isvalid() && RTTexture->IsLoaded()) RTTexture->Unload();
	RTTexture = NULL;
}
//---------------------------------------------------------------------

// Resolves RT to texture, if necessary
void CRenderTarget::Resolve()
{
	if (!ResolveToTexture) return;
	IDirect3DSurface9* pResolveSurface = NULL;
	n_assert(SUCCEEDED(RTTexture->GetD3D9Texture()->GetSurfaceLevel(0, &pResolveSurface)));
	n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->StretchRect(pRTSurface, NULL, pResolveSurface, NULL, D3DTEXF_NONE)));
	pResolveSurface->Release();
}
//---------------------------------------------------------------------

/*

void
D3D9RenderTarget::OnLostDevice()
{
	if (!this->isLosted)
	{
		this->isLosted = true;
		this->Discard();
	}
}

//------------------------------------------------------------------------------
void
D3D9RenderTarget::OnResetDevice()
{
	if (this->isLosted)
	{
		this->Setup();
		this->isLosted = false;
	}
}

*/
}