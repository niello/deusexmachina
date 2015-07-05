#include "D3D11Texture.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Core/Factory.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11Texture, 'TEX1', Render::CTexture);

bool CD3D11Texture::Create(ID3D11ShaderResourceView* pSRV)
{
	if (!pSRV) FAIL;
	ID3D11Resource* pTex = NULL;
	pSRV->GetResource(&pTex);
	bool Result = Create(pTex, pSRV);
	pTex->Release();
	return Result;
}
//---------------------------------------------------------------------

bool CD3D11Texture::Create(ID3D11Resource* pTexture, ID3D11ShaderResourceView* pSRV)
{
	if (!pTexture) FAIL;

	bool Result = false;
	ID3D11Texture1D* pTex1D = NULL;
	ID3D11Texture2D* pTex2D = NULL;
	ID3D11Texture3D* pTex3D = NULL;
	if (SUCCEEDED(pTexture->QueryInterface(&pTex2D)))
	{
		Result = Create(pTex2D, pSRV);
		pTex2D->Release();
	}
	else if (SUCCEEDED(pTexture->QueryInterface(&pTex3D)))
	{
		Result = Create(pTex3D, pSRV);
		pTex3D->Release();
	}
	else if (SUCCEEDED(pTexture->QueryInterface(&pTex1D)))
	{
		Result = Create(pTex1D, pSRV);
		pTex1D->Release();
	}

	return Result;
}
//---------------------------------------------------------------------

bool CD3D11Texture::Create(ID3D11Texture1D* pTexture, ID3D11ShaderResourceView* pSRV)
{
	if (!pTexture) FAIL;

	D3D11_TEXTURE1D_DESC D3DDesc;
	pTexture->GetDesc(&D3DDesc);

	Desc.Type = Texture_1D;
	Desc.Width = D3DDesc.Width;
	Desc.Height = 1;
	Desc.Depth = 1;
	Desc.MipLevels = D3DDesc.MipLevels;
	Desc.ArraySize = D3DDesc.ArraySize;
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = MSAA_None;

	Access.ResetTo(Access_GPU_Read); //???staging to?
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	pD3DTex = pTexture;
	pSRView = pSRV;
	D3DUsage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Texture::Create(ID3D11Texture2D* pTexture, ID3D11ShaderResourceView* pSRV)
{
	if (!pTexture) FAIL;

	D3D11_TEXTURE2D_DESC D3DDesc;
	pTexture->GetDesc(&D3DDesc);

	Desc.Type = (D3DDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? Texture_Cube : Texture_2D;
	Desc.Width = D3DDesc.Width;
	Desc.Height = D3DDesc.Height;
	Desc.Depth = 1;
	Desc.MipLevels = D3DDesc.MipLevels;
	Desc.ArraySize = D3DDesc.ArraySize;
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = CD3D11DriverFactory::D3DMSAAParamsToMSAAQuality(D3DDesc.SampleDesc);

	Access.ResetTo(Access_GPU_Read); //???staging to?
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	pD3DTex = pTexture;
	pSRView = pSRV;
	D3DUsage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11Texture::Create(ID3D11Texture3D* pTexture, ID3D11ShaderResourceView* pSRV)
{
	if (!pTexture) FAIL;

	D3D11_TEXTURE3D_DESC D3DDesc;
	pTexture->GetDesc(&D3DDesc);

	Desc.Type = Texture_3D;
	Desc.Width = D3DDesc.Width;
	Desc.Height = D3DDesc.Height;
	Desc.Depth = D3DDesc.Depth;
	Desc.MipLevels = D3DDesc.MipLevels;
	Desc.ArraySize = 1;
	Desc.Format = CD3D11DriverFactory::DXGIFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = MSAA_None;

	Access.ResetTo(Access_GPU_Read); //???staging to?
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	pD3DTex = pTexture;
	pSRView = pSRV;
	D3DUsage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

void CD3D11Texture::InternalDestroy()
{
	SAFE_RELEASE(pSRView);
	SAFE_RELEASE(pD3DTex);
}
//---------------------------------------------------------------------

}