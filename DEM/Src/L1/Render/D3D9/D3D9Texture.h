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

	IDirect3DBaseTexture9*	pD3DTex; //???or union?

	void MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags);

	//!!!only for D3DPOOL_DEFAULT! now manage in GPUDrv?
	//DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	//DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CD3D9Texture(): pD3DTex(NULL) {}
	virtual ~CD3D9Texture() { Destroy(); }

	bool						Create(IDirect3DBaseTexture9* pTexture);
	bool						Create(IDirect3DTexture9* pTexture);
	bool						Create(IDirect3DCubeTexture9* pTexture);
	bool						Create(IDirect3DVolumeTexture9* pTexture);
	virtual void				Destroy();

	virtual bool				IsResourceValid() const { return !!pD3DTex; }

	DWORD						GetPixelCount(bool IncludeMips) const; //???virtual?
	IDirect3DBaseTexture9*		GetD3DBaseTexture() const { /*n_assert(!LockCount);*/ return pD3DTex; }
	IDirect3DTexture9*			GetD3DTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_2D); return (IDirect3DTexture9*)pD3DTex; }
	IDirect3DCubeTexture9*		GetD3DCubeTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_Cube); return (IDirect3DCubeTexture9*)pD3DTex; }
	IDirect3DVolumeTexture9*	GetD3DVolumeTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_3D); return (IDirect3DVolumeTexture9*)pD3DTex; }
};

typedef Ptr<CD3D9Texture> PD3D9Texture;

}

#endif
