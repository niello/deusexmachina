#pragma once
#ifndef __DEM_L1_RENDER_D3D11_TEXTURE_H__
#define __DEM_L1_RENDER_D3D11_TEXTURE_H__

#include <Render/Texture.h>

// D3D11 implementation of VRAM texture

struct ID3D11Texture2D;

namespace Render
{

class CD3D11Texture: public CTexture
{
	__DeclareClass(CD3D11Texture);

protected:

	ID3D11Texture2D*	pD3DTex; //???store base texture?

public:

	CD3D11Texture();
	virtual ~CD3D11Texture(); // { if (IsLoaded()) Unload(); }

	bool				Create(ID3D11Texture2D* pTexture); // For internal use

	virtual bool		IsResourceValid() const { return !!pD3DTex; }

	ID3D11Texture2D*	GetD3DTexture() const { n_assert(!LockCount && Type == Texture2D); return pD3DTex; }
};

typedef Ptr<CD3D11Texture> PD3D11Texture;

}

#endif
