#pragma once
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
	FACTORY_CLASS_DECL;

protected:

	IDirect3DBaseTexture9*	pD3DTex = nullptr; //???or union?
	UINT					D3DUsage = {};
	D3DPOOL					D3DPool = {};

	void InternalDestroy();

public:

	virtual ~CD3D9Texture() { InternalDestroy(); }

	bool						Create(PTextureData Data, UINT Usage, D3DPOOL Pool, IDirect3DBaseTexture9* pTexture, bool HoldRAMCopy = false);
	virtual void				Destroy() { InternalDestroy(); CTexture::Destroy(); }

	virtual void                SetDebugName(std::string_view Name) override;

	IDirect3DBaseTexture9*		GetD3DBaseTexture() const { /*n_assert(!LockCount);*/ return pD3DTex; }
	IDirect3DTexture9*			GetD3DTexture() const;
	IDirect3DCubeTexture9*		GetD3DCubeTexture() const;
	IDirect3DVolumeTexture9*	GetD3DVolumeTexture() const;
	UINT						GetD3DUsage() const { return D3DUsage; }
	D3DPOOL						GetD3DPool() const { return D3DPool; }
};

typedef Ptr<CD3D9Texture> PD3D9Texture;

}
