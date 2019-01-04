#include "D3D9Texture.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/ImageUtils.h>
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

	RowPitch = CalcImageRowPitch(
		CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DDesc.Format),
		D3DDesc.Width,
		CD3D9DriverFactory::D3DFormatBlockSize(D3DDesc.Format) > 1);
	SlicePitch = 0;

	pD3DTex = pTexture;
	D3DUsage = D3DDesc.Usage;
	D3DPool = D3DDesc.Pool;
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

	RowPitch = CalcImageRowPitch(
		CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DDesc.Format),
		D3DDesc.Width,
		CD3D9DriverFactory::D3DFormatBlockSize(D3DDesc.Format) > 1);
	SlicePitch = 0;

	pD3DTex = pTexture;
	D3DUsage = D3DDesc.Usage;
	D3DPool = D3DDesc.Pool;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9Texture::Create(IDirect3DVolumeTexture9* pTexture)
{
	if (!pTexture) FAIL;

	D3DVOLUME_DESC D3DDesc;
	ZeroMemory(&D3DDesc, sizeof(D3DDesc));
	n_verify(SUCCEEDED(pTexture->GetLevelDesc(0, &D3DDesc)));

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

	bool IsBC = (CD3D9DriverFactory::D3DFormatBlockSize(D3DDesc.Format) > 1);
	RowPitch = CalcImageRowPitch(CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DDesc.Format), D3DDesc.Width, IsBC);
	SlicePitch = CalcImageSlicePitch(RowPitch, D3DDesc.Height, IsBC);

	pD3DTex = pTexture;
	D3DUsage = D3DDesc.Usage;
	D3DPool = D3DDesc.Pool;
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