#include "D3D11DepthStencilBuffer.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/TextureData.h>
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
	D3D11_DEPTH_STENCIL_VIEW_DESC DSDesc;
	pDSV->GetDesc(&DSDesc);
	if (DSDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2D &&
		DSDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DARRAY &&
		DSDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DMS &&
		DSDesc.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY)
	{
		FAIL;
	}
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(DSDesc.Format);

	ID3D11Resource* pTexRsrc = nullptr;
	pDSV->GetResource(&pTexRsrc);
	
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
	
	pTexRsrc->Release();

	if (Desc.UseAsShaderInput)
	{
		PTextureData TexData = n_new(CTextureData);

		CTextureDesc& TexDesc = TexData->Desc;
		TexDesc.Type = Texture_2D;
		TexDesc.Width = Desc.Width;
		TexDesc.Height = Desc.Height;
		TexDesc.Depth = 1;
		TexDesc.ArraySize = 1;
		TexDesc.MipLevels = 1;
		TexDesc.MSAAQuality = Desc.MSAAQuality;
		TexDesc.Format = Desc.Format;

		Texture = n_new(CD3D11Texture);
		if (!Texture->Create(TexData, D3DTexDesc.Usage, Access_GPU_Read, pTex, pSRV))
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
	Texture = nullptr;
	SAFE_RELEASE(pDSView);
}
//---------------------------------------------------------------------

CTexture* CD3D11DepthStencilBuffer::GetShaderResource() const
{
	return Texture.Get();
}
//---------------------------------------------------------------------

}