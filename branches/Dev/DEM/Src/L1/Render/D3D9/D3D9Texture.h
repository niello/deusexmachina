#pragma once
#ifndef __DEM_L1_RENDER_D3D9_TEXTURE_H__
#define __DEM_L1_RENDER_D3D9_TEXTURE_H__

#include <Render/Texture.h>
#include <Render/GPUResourceDefs.h>
#include <Render/RenderFwd.h>
#include <Events/EventsFwd.h>
#include <Data/Type.h>

// D3D9 implementation of VRAM texture

struct IDirect3DBaseTexture9;
struct IDirect3DTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;

namespace Render
{

class CD3D9Texture: public CTexture
{
	__DeclareClass(CD3D9Texture);

protected:

	IDirect3DBaseTexture9*	pD3DTex;

	union
	{
		IDirect3DTexture9*			pD3DTex2D;
		IDirect3DVolumeTexture9*	pD3DTex3D;
		IDirect3DCubeTexture9*		pD3DTexCube;
	};

	void MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags);

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	Data::CFlags	Access; //!!!can use as generic flags!

	//CD3D9Texture();
	//virtual ~CD3D9Texture(); // { if (IsLoaded()) Unload(); }

	bool			Create(IDirect3DTexture9* pTexture); // For internal use

	virtual bool	IsResourceValid() const { return !!pD3DTex; }

	//IDirect3DBaseTexture9*		GetD3DBaseTexture() const { n_assert(!LockCount); return pD3D9Tex; }
	IDirect3DTexture9*			GetD3DTexture() const { n_assert(!LockCount && Type == Texture2D); return pD3DTex2D; }
	//IDirect3DCubeTexture9*		GetD3DCubeTexture() const { n_assert(!LockCount && Type == TextureCube); return pD3D9TexCube; }
	//IDirect3DVolumeTexture9*	GetD3DVolumeTexture() const { n_assert(!LockCount && Type == Texture3D); return pD3D9Tex3D; }
};

typedef Ptr<CD3D9Texture> PD3D9Texture;

}

DECLARE_TYPE(Render::PTexture, 14)
#define TTexture DATA_TYPE(Render::PTexture)

#endif
