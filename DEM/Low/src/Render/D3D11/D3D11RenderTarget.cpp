#include "D3D11RenderTarget.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/TextureData.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D11RenderTarget, 'RT11', Render::CRenderTarget);

//!!!???assert destroyed?!
bool CD3D11RenderTarget::Create(ID3D11RenderTargetView* pRTV, ID3D11ShaderResourceView* pSRV)
{
	n_assert(pRTV);

	//???switch-case for tex types? only 2D for now
	D3D11_RENDER_TARGET_VIEW_DESC RTDesc;
	pRTV->GetDesc(&RTDesc);
	if (RTDesc.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2D &&
		RTDesc.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DARRAY &&
		RTDesc.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS &&
		RTDesc.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY) FAIL;
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(RTDesc.Format);

	ID3D11Resource* pTexRsrc = nullptr;
	pRTV->GetResource(&pTexRsrc);
	
	ID3D11Texture2D* pTex = nullptr;
	if (FAILED(pTexRsrc->QueryInterface<ID3D11Texture2D>(&pTex)))
	{
		pTexRsrc->Release();
		FAIL;
	}

	D3D11_TEXTURE2D_DESC D3DTexDesc;
	pTex->GetDesc(&D3DTexDesc);
	Desc.Width = D3DTexDesc.Width;
	Desc.Height = D3DTexDesc.Height;
	Desc.MSAAQuality = CD3D11DriverFactory::D3DMSAAParamsToMSAAQuality(D3DTexDesc.SampleDesc);
	Desc.UseAsShaderInput = !!pSRV; // (TexDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0;
	Desc.MipLevels = D3DTexDesc.MipLevels;
	
	pTexRsrc->Release();

	pRTView = pRTV;

	if (Desc.UseAsShaderInput)
	{
		PTextureData TexData = n_new(CTextureData);
		TexData->Desc = GetRenderTargetTextureDesc(Desc);

		Texture = n_new(CD3D11Texture);
		if (!Texture->Create(TexData, D3DTexDesc.Usage, Access_GPU_Read, pTex, pSRV))
		{
			pTex->Release();
			FAIL;
		}
	}
	else pTex->Release();

	OK;
}
//---------------------------------------------------------------------

void CD3D11RenderTarget::InternalDestroy()
{
	Texture = nullptr;
	SAFE_RELEASE(pRTView);
}
//---------------------------------------------------------------------

//???or GPUDriver method?
bool CD3D11RenderTarget::CopyResolveToTexture(PTexture Dest /*, region*/) const
{
	n_assert_dbg(Dest->IsA<CD3D11Texture>());

	if (!Dest) FAIL;
	ID3D11Texture2D* pDestTex = ((CD3D11Texture*)Dest.Get())->GetD3DTexture2D();

	ID3D11Device* pDev = nullptr;
	pDestTex->GetDevice(&pDev);

	ID3D11DeviceContext* pDevCtx = nullptr;
	pDev->GetImmediateContext(&pDevCtx);

	ID3D11Resource* pRsrc = nullptr;
	pRTView->GetResource(&pRsrc);

	if (Desc.MSAAQuality != MSAA_None) //!!! && Dest->GetDesc()->MSAAQuality == MSAA_None)!
	{
		pDevCtx->ResolveSubresource(pDestTex, 0, pRsrc, 0, CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format));
	}
	else //if (Desc.MSAAQuality == Dest->GetDesc()->MSAAQuality)
	{
		//pDevCtx->CopySubresourceRegion(pDestTex, 0, x, y, z, pRsrc, 0, pBox);
	}
	//???else FAIL;?

	pDestTex->Release();
	pDevCtx->Release();
	pDev->Release();

	OK;
}
//---------------------------------------------------------------------

CTexture* CD3D11RenderTarget::GetShaderResource() const
{
	//???resolve to single-AA texture with possible mips, or may reuse as multisapled input?
	//can resolve manually when needed using CopyResolveToTexture()!
	//!!!on render to texture end, if MipLevels > 0, generate mips!
	return Texture.Get();
}
//---------------------------------------------------------------------

void CD3D11RenderTarget::SetDebugName(std::string_view Name)
{
#if DEM_RENDER_DEBUG
	if (pRTView) pRTView->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)Name.size(), Name.data());
	if (Texture) Texture->SetDebugName(Name);
#endif
}
//---------------------------------------------------------------------

}
