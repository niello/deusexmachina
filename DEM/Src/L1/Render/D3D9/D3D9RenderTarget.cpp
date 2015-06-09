#include "D3D9RenderTarget.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{

bool CD3D9RenderTarget::Create(IDirect3DSurface9* pSurface)
{
	n_assert(pSurface);

	D3DSURFACE_DESC RTDesc;
	if (FAILED(pRTSurface->GetDesc(&RTDesc)) || !(RTDesc.Usage & D3DUSAGE_RENDERTARGET)) FAIL;

	Desc.Width = RTDesc.Width;
	Desc.Height = RTDesc.Height;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(RTDesc.Format);
	Desc.MSAAQuality = CD3D9DriverFactory::D3DMSAAParamsToMSAAQuality(RTDesc.MultiSampleType, RTDesc.MultiSampleQuality);
	Desc.UseAsShaderInput = false; // As no texture associated

	pRTSurface = pSurface;
	RTTexture = NULL;

	//!!!process GPU resources in driver!
	//SUBSCRIBE_PEVENT(OnRenderDeviceRelease, CRenderTarget, OnDeviceRelease);
	//SUBSCRIBE_PEVENT(OnRenderDeviceLost, CRenderTarget, OnDeviceLost);
	//SUBSCRIBE_PEVENT(OnRenderDeviceReset, CRenderTarget, OnDeviceReset);

	OK;
}
//---------------------------------------------------------------------

void CD3D9RenderTarget::Destroy()
{
	//!!!process GPU resources in driver!
	//UNSUBSCRIBE_EVENT(OnRenderDeviceRelease);
	//UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
	//UNSUBSCRIBE_EVENT(OnRenderDeviceReset);

	SAFE_RELEASE(pRTSurface);
	if (RTTexture.IsValid() && RTTexture->IsLoaded()) RTTexture->Unload();
	RTTexture = NULL;
}
//---------------------------------------------------------------------

bool CRenderTarget::Create(CStrID TextureID, EPixelFormat RTFormat, EPixelFormat DSFormat, float Width, float Height,
						   bool AbsWH, EMSAAQuality MSAA, DWORD TexWidth, DWORD TexHeight, bool UseAutoDS
						   /*, use mips*/)
{
	if (!TextureID.IsValid() || RTFormat == D3DFMT_UNKNOWN) FAIL;

	//???assert not created?

	IsDefaultRT = false;

	W = Width;
	H = Height;
	AbsoluteWH = AbsWH;
	MSAAQuality = MSAA;
	TexW = TexWidth;
	TexH = TexHeight;

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
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//???instead store windowed display mode and fullscreen display mode?
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		AbsWidth = (DWORD)(Width * (float)RenderSrv->GetBackBufferWidth());
		AbsHeight = (DWORD)(Height * (float)RenderSrv->GetBackBufferHeight());
	}

	RTTexture = RenderSrv->TextureMgr.GetOrCreateTypedResource(TextureID);
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

	if (UseAutoDS)
	{
		n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->GetDepthStencilSurface(&pDSSurface)));
		DSFmt = D3DFMT_UNKNOWN;
	}
	else if (DSFormat != D3DFMT_UNKNOWN)
	{
		n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->CreateDepthStencilSurface(
			AbsWidth,
			AbsHeight,
			DSFormat,
			D3DMSAAType,
			D3DMSAAQuality,
			TRUE,
			&pDSSurface,
			NULL)));

		DSFmt = DSFormat;
	}

	SUBSCRIBE_PEVENT(OnRenderDeviceRelease, CRenderTarget, OnDeviceRelease);
	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CRenderTarget, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CRenderTarget, OnDeviceReset);

	OK;
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

bool CRenderTarget::OnDeviceRelease(const Events::CEventBase& Ev)
{
	Destroy();
	OK;
}
//---------------------------------------------------------------------

bool CRenderTarget::OnDeviceLost(const Events::CEventBase& Ev)
{
	UNSUBSCRIBE_EVENT(OnRenderDeviceRelease);
	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);

	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface); //???what if shared? may AddRef in Create
	if (RTTexture.IsValid() && RTTexture->IsLoaded()) RTTexture->Unload();

	OK;
}
//---------------------------------------------------------------------

bool CRenderTarget::OnDeviceReset(const Events::CEventBase& Ev)
{
	if (IsDefaultRT) { n_assert(CreateDefaultRT()); }
	else n_assert(Create(	RTTexture->GetUID(), RTFmt, DSFmt, W, H, AbsoluteWH,
							MSAAQuality, TexW, TexH, (DSFmt == D3DFMT_UNKNOWN)));
	OK;
}
//---------------------------------------------------------------------

}