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
//	if (RTTexture.IsValid() && RTTexture->IsLoaded()) RTTexture->Unload();
	RTTexture = NULL;
}
//---------------------------------------------------------------------

/*
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
*/

}