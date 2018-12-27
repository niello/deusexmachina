#pragma once
#ifndef __DEM_L1_RENDER_D3D9_TEXTURE_H__
#define __DEM_L1_RENDER_D3D9_TEXTURE_H__

#include <Render/Texture.h>

// D3D9 implementation of VRAM texture

struct IDirect3DBaseTexture9;
struct IDirect3DTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;
typedef enum _D3DPOOL D3DPOOL;
typedef unsigned int UINT;

namespace Render
{

class CD3D9Texture: public CTexture
{
	__DeclareClass(CD3D9Texture);

protected:

	IDirect3DBaseTexture9*	pD3DTex; //???or union?
	UINT					D3DUsage;
	D3DPOOL					D3DPool;

	void InternalDestroy();

public:

	CD3D9Texture(): pD3DTex(NULL) {}
	virtual ~CD3D9Texture() { InternalDestroy(); }

	bool						Create(IDirect3DBaseTexture9* pTexture);
	bool						Create(IDirect3DTexture9* pTexture);
	bool						Create(IDirect3DCubeTexture9* pTexture);
	bool						Create(IDirect3DVolumeTexture9* pTexture);
	virtual void				Destroy() { InternalDestroy(); }

	virtual bool				IsResourceValid() const { return !!pD3DTex; }

	IDirect3DBaseTexture9*		GetD3DBaseTexture() const { /*n_assert(!LockCount);*/ return pD3DTex; }
	IDirect3DTexture9*			GetD3DTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_2D); return (IDirect3DTexture9*)pD3DTex; }
	IDirect3DCubeTexture9*		GetD3DCubeTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_Cube); return (IDirect3DCubeTexture9*)pD3DTex; }
	IDirect3DVolumeTexture9*	GetD3DVolumeTexture() const { n_assert(/*!LockCount &&*/ Desc.Type == Texture_3D); return (IDirect3DVolumeTexture9*)pD3DTex; }
	UINT						GetD3DUsage() const { return D3DUsage; }
	D3DPOOL						GetD3DPool() const { return D3DPool; }
};

typedef Ptr<CD3D9Texture> PD3D9Texture;

}

#endif
