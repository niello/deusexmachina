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
	Desc.Depth = 0;
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
	Desc.Depth = 0;
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
	OK;
}
//---------------------------------------------------------------------

void CD3D9Texture::InternalDestroy()
{
//	n_assert(!LockCount);
	SAFE_RELEASE(pD3DTex);
}
//---------------------------------------------------------------------

//inline void CTexture::MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags)
//{
//	switch (MapType)
//	{
//		//!!!Map_Setup!
//
//		case Map_Read:
//			n_assert((Usage_Dynamic == Usage) && (CPU_Read == Access));
//			LockFlags |= D3DLOCK_READONLY;
//			break;
//		case Map_Write:
//			n_assert((Usage_Dynamic == Usage) && (CPU_Write == Access));
//			break;
//		case Map_ReadWrite:
//			n_assert((Usage_Dynamic == Usage) && (CPU_ReadWrite == Access));
//			break;
//		case Map_WriteDiscard:
//			n_assert((Usage_Dynamic == Usage) && (CPU_Write == Access));
//			LockFlags |= D3DLOCK_DISCARD;
//			break;
//	}
//}
////---------------------------------------------------------------------
//
//bool CTexture::Map(int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
//{
//	n_assert((Type == Texture2D || Type == Texture3D) && MapType != Map_WriteNoOverwrite);
//
//	DWORD LockFlags = 0;
//	MapTypeToLockFlags(MapType, LockFlags);
//
//	if (Type == Texture2D)
//	{
//		D3DLOCKED_RECT LockedRect = { 0 };
//		if (SUCCEEDED(GetD3D9Texture()->LockRect(MipLevel, &LockedRect, NULL, LockFlags)))
//		{
//			OutMapInfo.pData = LockedRect.pBits;
//			OutMapInfo.RowPitch = LockedRect.Pitch;
//			OutMapInfo.DepthPitch = 0;
//			LockCount++;
//			OK;
//		}
//	}
//	else if (Type == Texture3D)
//	{
//		D3DLOCKED_BOX LockedBox = { 0 };
//		if (SUCCEEDED(GetD3D9VolumeTexture()->LockBox(MipLevel, &LockedBox, NULL, LockFlags)))
//		{
//			OutMapInfo.pData = LockedBox.pBits;
//			OutMapInfo.RowPitch = LockedBox.RowPitch;
//			OutMapInfo.DepthPitch = LockedBox.SlicePitch;
//			LockCount++;
//			OK;
//		}
//	}
//
//	FAIL;
//}
////---------------------------------------------------------------------
//
//void CTexture::Unmap(int MipLevel)
//{
//	n_assert(LockCount > 0);
//	if (Texture2D == Type) pD3D9Tex2D->UnlockRect(MipLevel);
//	else if (Texture3D == Type) pD3D9Tex3D->UnlockBox(MipLevel);
//	else Sys::Error("CTexture::Unmap -> Cube texture, use UnmapCubeFace");
//	LockCount--;
//}
////---------------------------------------------------------------------
//
//bool CTexture::MapCubeFace(ECubeFace Face, int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
//{
//	n_assert(Type == TextureCube && MapType != Map_WriteNoOverwrite);
//
//	DWORD LockFlags = D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK;
//	MapTypeToLockFlags(MapType, LockFlags);
//
//	D3DLOCKED_RECT LockedRect = { 0 };
//	if (FAILED(GetD3D9CubeTexture()->LockRect((D3DCUBEMAP_FACES)Face, MipLevel, &LockedRect, NULL, LockFlags))) FAIL;
//
//	OutMapInfo.pData = LockedRect.pBits;
//	OutMapInfo.RowPitch = LockedRect.Pitch;
//	OutMapInfo.DepthPitch = 0;
//	LockCount++;
//	OK;
//}
////---------------------------------------------------------------------
//
//void CTexture::UnmapCubeFace(ECubeFace Face, int MipLevel)
//{
//	n_assert(Type == TextureCube && LockCount > 0);
//	GetD3D9CubeTexture()->UnlockRect((D3DCUBEMAP_FACES)Face, MipLevel);
//	LockCount--;
//}
////---------------------------------------------------------------------
//
//bool CTexture::OnDeviceLost(const Events::CEventBase& Ev)
//{
//	if (IsLoaded()) Unload(); //!!!will unsubscribe OnDeviceReset!
//	OK;
//}
////---------------------------------------------------------------------
//
///*
//bool CTexture::OnDeviceReset(const Events::CEventBase& Ev)
//{
//	//???reload? or recreate, need to set flag 'I am valid, but empty'. Or leave recreation to the resource owner.
//	OK;
//}
////---------------------------------------------------------------------
//*/

}