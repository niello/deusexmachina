#include "D3D9Texture.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9Texture, 'TEX9', Render::CTexture);

bool CD3D9Texture::Create(IDirect3DBaseTexture9* pTexture)
{
	if (!pTexture) FAIL;

	bool Result = false;
	IDirect3DTexture9* pTex = NULL;
	IDirect3DCubeTexture9* pTexC = NULL;
	IDirect3DVolumeTexture9* pTexV = NULL;
	if (SUCCEEDED(pTexture->QueryInterface(IID_IDirect3DTexture9, (void**)&pTex)))
	{
		Result = Create(pTex);
		pTex->Release();
	}
	else if (SUCCEEDED(pTexture->QueryInterface(IID_IDirect3DCubeTexture9, (void**)&pTexC)))
	{
		Result = Create(pTexC);
		pTexC->Release();
	}
	else if (SUCCEEDED(pTexture->QueryInterface(IID_IDirect3DVolumeTexture9, (void**)&pTexV)))
	{
		Result = Create(pTexV);
		pTexV->Release();
	}

	return Result;
}
//---------------------------------------------------------------------

bool CD3D9Texture::Create(IDirect3DTexture9* pTexture)
{
	if (!pTexture) FAIL;

	D3DSURFACE_DESC D3DDesc;
	ZeroMemory(&D3DDesc, sizeof(Desc));
	if (FAILED(pTexture->GetLevelDesc(0, &D3DDesc))) FAIL;

	Desc.Type = Texture_2D;
	Desc.Width = D3DDesc.Width;
	Desc.Height = D3DDesc.Height;
	Desc.Depth = 1;
	Desc.MipLevels = pTexture->GetLevelCount();
	Desc.ArraySize = 1;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = MSAA_None;

	Access.ClearAll();
	if (D3DDesc.Pool == D3DPOOL_SYSTEMMEM || D3DDesc.Pool == D3DPOOL_SCRATCH)
	{
		Access.Set(Access_CPU_Read | Access_CPU_Write);
	}
	else
	{
		Access.Set(Access_GPU_Read);
		if (D3DDesc.Usage & D3DUSAGE_DYNAMIC) Access.Set(Access_CPU_Write);
		else Access.Set(Access_GPU_Write);
	}

	pD3DTex = pTexture;
	Usage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9Texture::Create(IDirect3DCubeTexture9* pTexture)
{
	if (!pTexture) FAIL;

	D3DSURFACE_DESC D3DDesc;
	ZeroMemory(&D3DDesc, sizeof(Desc));
	if (FAILED(pTexture->GetLevelDesc(0, &D3DDesc))) FAIL;

	Desc.Type = Texture_Cube;
	Desc.Width = D3DDesc.Width;
	Desc.Height = D3DDesc.Height;
	Desc.Depth = 1;
	Desc.MipLevels = pTexture->GetLevelCount();
	Desc.ArraySize = 1;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = MSAA_None;

	Access.ClearAll();
	if (D3DDesc.Pool == D3DPOOL_SYSTEMMEM || D3DDesc.Pool == D3DPOOL_SCRATCH)
	{
		Access.Set(Access_CPU_Read | Access_CPU_Write);
	}
	else
	{
		Access.Set(Access_GPU_Read);
		if (D3DDesc.Usage & D3DUSAGE_DYNAMIC) Access.Set(Access_CPU_Write);
		else Access.Set(Access_GPU_Write);
	}

	pD3DTex = pTexture;
	Usage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9Texture::Create(IDirect3DVolumeTexture9* pTexture)
{
	if (!pTexture) FAIL;

	D3DVOLUME_DESC D3DDesc;
	ZeroMemory(&D3DDesc, sizeof(Desc));
	n_assert(SUCCEEDED(pTexture->GetLevelDesc(0, &D3DDesc)));

	Desc.Type = Texture_2D;
	Desc.Width = D3DDesc.Width;
	Desc.Height = D3DDesc.Height;
	Desc.Depth = D3DDesc.Depth;
	Desc.MipLevels = pTexture->GetLevelCount();
	Desc.ArraySize = 1;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDesc.Format);
	Desc.MSAAQuality = MSAA_None;

	Access.ClearAll();
	if (D3DDesc.Pool == D3DPOOL_SYSTEMMEM || D3DDesc.Pool == D3DPOOL_SCRATCH)
	{
		Access.Set(Access_CPU_Read | Access_CPU_Write);
	}
	else
	{
		Access.Set(Access_GPU_Read);
		if (D3DDesc.Usage & D3DUSAGE_DYNAMIC) Access.Set(Access_CPU_Write);
		else Access.Set(Access_GPU_Write);
	}

	pD3DTex = pTexture;
	Usage = D3DDesc.Usage;
	OK;
}
//---------------------------------------------------------------------

void CD3D9Texture::InternalDestroy()
{
//	n_assert(!LockCount);
	SAFE_RELEASE(pD3DTex);
}
//---------------------------------------------------------------------

}