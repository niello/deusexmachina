#include "Texture.h"

#include <Resources/ResourceServer.h>
#include <d3d9.h>

namespace Render
{
ImplementRTTI(Render::CTexture, Resources::CResource);

//CTexture::~CTexture()
//{
//}
////---------------------------------------------------------------------

inline void CTexture::MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags)
{
	switch (MapType)
	{
		case MapRead:
			n_assert((UsageDynamic == Usage) && (AccessRead == AccessMode));
			LockFlags |= D3DLOCK_READONLY;
			break;
		case MapWrite:
			n_assert((UsageDynamic == Usage) && (AccessWrite == AccessMode));
			break;
		case MapReadWrite:
			n_assert((UsageDynamic == Usage) && (AccessReadWrite == AccessMode));
			break;
		case MapWriteDiscard:
			n_assert((UsageDynamic == Usage) && (AccessWrite == AccessMode));
			LockFlags |= D3DLOCK_DISCARD;
			break;
	}
}
//---------------------------------------------------------------------

void CTexture::Unload()
{
	n_assert(!LockCount);
	SAFE_RELEASE(pD3D9Tex);
	CResource::Unload();
}
//---------------------------------------------------------------------

bool CTexture::Map(int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
{
	n_assert((Type == Texture2D || Type == Texture3D) && MapType != MapWriteNoOverwrite);

	DWORD LockFlags = 0;
	MapTypeToLockFlags(MapType, LockFlags);

	if (Type == Texture2D)
	{
		D3DLOCKED_RECT LockedRect = { 0 };
		if (SUCCEEDED(GetD3D9Texture()->LockRect(MipLevel, &LockedRect, NULL, LockFlags)))
		{
			OutMapInfo.pData = LockedRect.pBits;
			OutMapInfo.RowPitch = LockedRect.Pitch;
			OutMapInfo.DepthPitch = 0;
			LockCount++;
			OK;
		}
	}
	else if (Type == Texture3D)
	{
		D3DLOCKED_BOX LockedBox = { 0 };
		if (SUCCEEDED(GetD3D9VolumeTexture()->LockBox(MipLevel, &LockedBox, NULL, LockFlags)))
		{
			OutMapInfo.pData = LockedBox.pBits;
			OutMapInfo.RowPitch = LockedBox.RowPitch;
			OutMapInfo.DepthPitch = LockedBox.SlicePitch;
			LockCount++;
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

void CTexture::Unmap(int MipLevel)
{
	n_assert(LockCount > 0);
	if (Texture2D == Type) GetD3D9Texture()->UnlockRect(MipLevel);
	else if (Texture3D == Type) GetD3D9VolumeTexture()->UnlockBox(MipLevel);
	else n_error("CTexture::Unmap -> Cube texture, use UnmapCubeFace");
	LockCount--;
}
//---------------------------------------------------------------------

bool CTexture::MapCubeFace(ECubeFace Face, int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
{
	n_assert(Type == TextureCube && MapType != MapWriteNoOverwrite);

	DWORD LockFlags = D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK;
	MapTypeToLockFlags(MapType, LockFlags);

	D3DLOCKED_RECT LockedRect = { 0 };
	if (FAILED(GetD3D9CubeTexture()->LockRect((D3DCUBEMAP_FACES)Face, MipLevel, &LockedRect, NULL, LockFlags))) FAIL;

	OutMapInfo.pData = LockedRect.pBits;
	OutMapInfo.RowPitch = LockedRect.Pitch;
	OutMapInfo.DepthPitch = 0;
	LockCount++;
	OK;
}
//---------------------------------------------------------------------

void CTexture::UnmapCubeFace(ECubeFace Face, int MipLevel)
{
	n_assert(Type == TextureCube && LockCount > 0);
	GetD3D9CubeTexture()->UnlockRect((D3DCUBEMAP_FACES)Face, MipLevel);
	LockCount--;
}
//---------------------------------------------------------------------

void CTexture::SetupFromD3D9Texture(IDirect3DTexture9* pTex, bool SetLoaded)
{
	n_assert(pTex);    

	// Need to query for base interface under Win32
	pD3D9Tex2D = pTex;
	n_assert(SUCCEEDED(pD3D9Tex2D->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

	Type = Texture2D;
	D3DSURFACE_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));
	n_assert(SUCCEEDED(pTex->GetLevelDesc(0, &Desc)));
	Width = Desc.Width;
	Height = Desc.Height;
	Depth = 1;
	MipCount = pTex->GetLevelCount();
	PixelFormat = AsNebulaPixelFormat(Desc.Format);
	if (SetLoaded) State = Resources::Rsrc_Loaded;
}
//---------------------------------------------------------------------

void CTexture::SetupFromD3D9VolumeTexture(IDirect3DVolumeTexture9* pTex, bool SetLoaded)
{
	n_assert(pTex);

	pD3D9Tex3D = pTex;
	n_assert(SUCCEEDED(pTex->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

	Type = Texture3D;
	D3DVOLUME_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));
	n_assert(SUCCEEDED(pTex->GetLevelDesc(0, &Desc)));
	Width = Desc.Width;
	Height = Desc.Height;
	Depth = Desc.Depth;
	MipCount = pTex->GetLevelCount();
	PixelFormat = AsNebulaPixelFormat(Desc.Format);
	if (SetLoaded) State = Resources::Rsrc_Loaded;
}
//---------------------------------------------------------------------

void CTexture::SetupFromD3D9CubeTexture(IDirect3DCubeTexture9* pTex, bool SetLoaded)
{
	n_assert(pTex);

	pD3D9TexCube = pTex;
	n_assert(SUCCEEDED(pTex->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

	Type = TextureCube;
	D3DSURFACE_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));
	n_assert(SUCCEEDED(pTex->GetLevelDesc(0, &Desc)));
	Width = Desc.Width;
	Height = Desc.Height;
	MipCount = pTex->GetLevelCount();
	PixelFormat = AsNebulaPixelFormat(Desc.Format);
	if (SetLoaded) State = Resources::Rsrc_Loaded;
}
//---------------------------------------------------------------------

}