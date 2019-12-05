#include "D3D9RenderTarget.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9Texture.h>
#include <Core/Factory.h>
#include "DEMD3D9.h"

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D9RenderTarget, 'RT09', Render::CRenderTarget);

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
		IDirect3DSurface9* pTmpSurf = nullptr;
		if (FAILED(Texture->GetD3DTexture()->GetSurfaceLevel(0, &pTmpSurf))) FAIL;
		NeedResolve = (pTmpSurf != pSurface);
		Desc.UseAsShaderInput = true;
		Desc.MipLevels = Texture->GetD3DTexture()->GetLevelCount();
		SRTexture = Texture;
	}
	else
	{
		NeedResolve = false;
		Desc.UseAsShaderInput = false;
		Desc.MipLevels = 1;
		SRTexture = nullptr;
	}

	pRTSurface = pSurface;

	//!!!process GPU resources in driver!
	//SUBSCRIBE_PEVENT(OnRenderDeviceRelease, CRenderTarget, OnDeviceRelease);
	//SUBSCRIBE_PEVENT(OnRenderDeviceLost, CRenderTarget, OnDeviceLost);
	//SUBSCRIBE_PEVENT(OnRenderDeviceReset, CRenderTarget, OnDeviceReset);

	OK;
}
//---------------------------------------------------------------------

void CD3D9RenderTarget::InternalDestroy()
{
	//!!!process GPU resources in driver!
	//UNSUBSCRIBE_EVENT(OnRenderDeviceRelease);
	//UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
	//UNSUBSCRIBE_EVENT(OnRenderDeviceReset);

	SAFE_RELEASE(pRTSurface);
//	if (SRTexture.IsValid() && SRTexture->IsLoaded()) RTTexture->Unload();
	SRTexture = nullptr;
}
//---------------------------------------------------------------------

//???or GPUDriver method?
bool CD3D9RenderTarget::CopyResolveToTexture(PTexture Dest /*, region*/) const
{
	n_assert_dbg(Dest->IsA<CD3D9Texture>());

	if (Dest.IsNullPtr()) FAIL;
	IDirect3DTexture9* pDestTex = ((CD3D9Texture*)Dest.Get())->GetD3DTexture();

	IDirect3DDevice9* pDev = nullptr;
	if (FAILED(pDestTex->GetDevice(&pDev))) FAIL;

	IDirect3DSurface9* pResolveSurface = nullptr;
	if (FAILED(pDestTex->GetSurfaceLevel(0, &pResolveSurface)))
	{
		pDev->Release();
		FAIL;
	}

	bool Result = SUCCEEDED(pDev->StretchRect(pRTSurface, nullptr, pResolveSurface, nullptr, D3DTEXF_NONE));
	pResolveSurface->Release();
	pDev->Release();

	return Result;
}
//---------------------------------------------------------------------

CTexture* CD3D9RenderTarget::GetShaderResource() const
{
	//???!!!detect need autoresolve?! or call resolve at render phase end? RT::OnRenderingComplete()
	//!!!on render to texture end, if MipLevels > 0, generate mips!
	//autogen mipmap flag is set, but will it work? if will, mips will be generated only as required by driver
	return SRTexture.Get();
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