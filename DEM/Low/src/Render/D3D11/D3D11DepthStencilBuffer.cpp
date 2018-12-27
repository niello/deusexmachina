#include "D3D11DepthStencilBuffer.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11DepthStencilBuffer, 'DSB1', Render::CDepthStencilBuffer);

//!!!???assert destroyed?!
bool CD3D11DepthStencilBuffer::Create(ID3D11DepthStencilView* pDSV, ID3D11ShaderResourceView* pSRV)
{
	n_assert(pDSV);

	//???switch-case for tex types? only 2D for now
	D3D11_DEPTH_STENCIL_VIEW_DESC RTDesc;
	pDSV->GetDesc(&RTDesc);
	if (RTDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2D &&
		RTDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DARRAY &&
		RTDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DMS &&
		RTDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY) FAIL;
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(RTDesc.Format);

	ID3D11Resource* pTexRsrc = NULL;
	pDSV->GetResource(&pTexRsrc);
	
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
	Desc.UseAsShaderInput = !!pSRV; // (TexDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0;
	
	pTexRsrc->Release();

	if (Desc.UseAsShaderInput)
	{
		Texture = n_new(CD3D11Texture);
		if (!Texture->Create(pTex, pSRV))
		{
			pTex->Release();
			FAIL;
		}
	}
	else pTex->Release();

	pDSView = pDSV;
	OK;
}
//---------------------------------------------------------------------

void CD3D11DepthStencilBuffer::InternalDestroy()
{
	Texture = NULL;
	SAFE_RELEASE(pDSView);
}
//---------------------------------------------------------------------

CTexture* CD3D11DepthStencilBuffer::GetShaderResource() const
{
	return Texture.GetUnsafe();
}
//---------------------------------------------------------------------

}