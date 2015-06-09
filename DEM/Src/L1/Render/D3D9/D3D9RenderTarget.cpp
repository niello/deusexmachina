#include "D3D9RenderTarget.h"

#include <Render/RenderServer.h>
#include <Events/EventServer.h>

namespace Render
{

bool CRenderTarget::CreateDefaultRT()
{
	n_assert(!RTTexture.IsValid());
	//???assert not created? empty texture is not enough!

	IsDefaultRT = true;

	const CDisplayMode& DispMode = RenderSrv->GetDisplay().GetDisplayMode();
	//float Width = RenderSrv->GetBackBufferWidth();
	//float Height = RenderSrv->GetBackBufferHeight();
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

	SUBSCRIBE_PEVENT(OnRenderDeviceRelease, CRenderTarget, OnDeviceRelease);
	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CRenderTarget, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CRenderTarget, OnDeviceReset);

	OK;
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

void CRenderTarget::Destroy()
{
	UNSUBSCRIBE_EVENT(OnRenderDeviceRelease);
	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
	UNSUBSCRIBE_EVENT(OnRenderDeviceReset);

	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface); //???what if shared? may AddRef in Create
	if (RTTexture.IsValid() && RTTexture->IsLoaded()) RTTexture->Unload();
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