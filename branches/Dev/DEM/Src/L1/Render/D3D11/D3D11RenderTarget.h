#pragma once
#ifndef __DEM_L1_D3D11_RENDER_TARGET_H__
#define __DEM_L1_D3D11_RENDER_TARGET_H__

#include <Render/RenderTarget.h>

// D3D11 implementation of a render target

struct ID3D11Texture2D;
struct ID3D11RenderTargetView;

namespace Render
{
//typedef Ptr<class CTexture> PTexture;

class CD3D11RenderTarget: public CRenderTarget
{
protected:

	ID3D11RenderTargetView* pRTView;
	PTexture				RTTexture;

public:

	CD3D11RenderTarget(): pRTView(NULL) {}

	bool				Create(ID3D11Texture2D* pTexture, ID3D11RenderTargetView* pRTV); // For internal use
	void				Destroy();

	virtual CTexture*	GetShaderResource() const { return NULL; } //???or store PTexture inside always and avoid virtualization here?
};

typedef Ptr<CD3D11RenderTarget> PD3D11RenderTarget;

}

#endif
