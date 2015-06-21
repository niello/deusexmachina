#include "D3D11RenderTarget.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11Texture.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{

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
	Desc.UseAsShaderInput = !!pSRV;

	ID3D11Resource* pTexRsrc = NULL;
	pRTV->GetResource(&pTexRsrc);
	
	ID3D11Texture2D* pTex = NULL;
	if (FAILED(pTexRsrc->QueryInterface<ID3D11Texture2D>(&pTex)))
	{
		pTexRsrc->Release();
		FAIL;
	}

	D3D11_TEXTURE2D_DESC TexDesc;
	pTex->GetDesc(&TexDesc);
	Desc.Width = TexDesc.Width;
	Desc.Height = TexDesc.Height;
	Desc.MSAAQuality = CD3D11DriverFactory::D3DMSAAParamsToMSAAQuality(TexDesc.SampleDesc);
	
	pTexRsrc->Release();

	pRTView = pRTV;
	pSRView = pSRV;
	if (pSRV)
	{
		Texture = n_new(CD3D11Texture);
		if (!Texture->Create(pTex))
		{
			pTex->Release();
			FAIL;
		}
	}
	else pTex->Release();

	OK;
}
//---------------------------------------------------------------------

void CD3D11RenderTarget::Destroy()
{
}
//---------------------------------------------------------------------

//???or GPUDriver method?
bool CD3D11RenderTarget::CopyResolveToTexture(PTexture Dest /*, region*/) const
{
	n_assert_dbg(Dest->IsA<CD3D11Texture>());

	if (!Dest.IsValid()) FAIL;
	ID3D11Texture2D* pDestTex = ((CD3D11Texture*)Dest.GetUnsafe())->GetD3DTexture();

	ID3D11Device* pDev = NULL;
	pDestTex->GetDevice(&pDev);

	ID3D11DeviceContext* pDevCtx = NULL;
	pDev->GetImmediateContext(&pDevCtx);

	ID3D11Resource* pRsrc = NULL;
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
	return Texture.GetUnsafe();
}
//---------------------------------------------------------------------

}