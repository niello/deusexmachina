#include "D3D9RenderTarget.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9Texture.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{

//!!!???assert destroyed?!
bool CD3D9RenderTarget::Create(IDirect3DSurface9* pSurface, PD3D9Texture Texture)
{
	n_assert(pSurface);

	D3DSURFACE_DESC RTDesc;
	if (FAILED(pSurface->GetDesc(&RTDesc)) || !(RTDesc.Usage & D3DUSAGE_RENDERTARGET)) FAIL;

	Desc.Width = RTDesc.Width;
	Desc.Height = RTDesc.Height;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(RTDesc.Format);
	Desc.MSAAQuality = CD3D9DriverFactory::D3DMSAAParamsToMSAAQuality(RTDesc.MultiSampleType, RTDesc.MultiSampleQuality);

	if (Texture.IsValidPtr())
	{
		IDirect3DSurface9* pTmpSurf = NULL;
		if (FAILED(Texture->GetD3DTexture()->GetSurfaceLevel(0, &pTmpSurf))) FAIL;
		NeedResolve = (pTmpSurf != pSurface);
		Desc.UseAsShaderInput = true;
		SRTexture = Texture;
	}
	else
	{
		NeedResolve = false;
		Desc.UseAsShaderInput = false;
		SRTexture = NULL;
	}

	pRTSurface = pSurface;

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
//	if (SRTexture.IsValid() && SRTexture->IsLoaded()) RTTexture->Unload();
	SRTexture = NULL;
}
//---------------------------------------------------------------------

//???or GPUDriver method?
bool CD3D9RenderTarget::CopyResolveToTexture(PTexture Dest /*, region*/) const
{
	n_assert_dbg(Dest->IsA<CD3D9Texture>());

	if (Dest.IsNullPtr()) FAIL;
	IDirect3DTexture9* pDestTex = ((CD3D9Texture*)Dest.GetUnsafe())->GetD3DTexture();

	IDirect3DDevice9* pDev = NULL;
	if (FAILED(pDestTex->GetDevice(&pDev))) FAIL;

	IDirect3DSurface9* pResolveSurface = NULL;
	if (FAILED(pDestTex->GetSurfaceLevel(0, &pResolveSurface)))
	{
		pDev->Release();
		FAIL;
	}

	bool Result = SUCCEEDED(pDev->StretchRect(pRTSurface, NULL, pResolveSurface, NULL, D3DTEXF_NONE));
	pResolveSurface->Release();
	pDev->Release();

	return Result;
}
//---------------------------------------------------------------------

CTexture* CD3D9RenderTarget::GetShaderResource() const
{
	//???!!!detect need autoresolve?! or call resolve at render phase end? RT::OnRenderingComplete()
	return SRTexture.GetUnsafe();
}
//---------------------------------------------------------------------

/*
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
*/

}