#pragma once
#ifndef __DEM_L1_D3D11_RENDER_TARGET_H__
#define __DEM_L1_D3D11_RENDER_TARGET_H__

#include <Render/RenderTarget.h>

// D3D11 implementation of a render target

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace Render
{
typedef Ptr<class CD3D11Texture> PD3D11Texture;

class CD3D11RenderTarget: public CRenderTarget
{
	__DeclareClass(CD3D11RenderTarget);

protected:

	ID3D11RenderTargetView*		pRTView;
	PD3D11Texture				Texture;

	void					InternalDestroy();

public:

	CD3D11RenderTarget(): pRTView(nullptr) {}
	virtual ~CD3D11RenderTarget() { InternalDestroy(); }

	bool					Create(ID3D11RenderTargetView* pRTV, ID3D11ShaderResourceView* pSRV); // For internal use
	virtual void			Destroy() { InternalDestroy(); }
	virtual bool			IsValid() const { return !!pRTView; }
	virtual bool			CopyResolveToTexture(PTexture Dest /*, region*/) const;
	virtual CTexture*		GetShaderResource() const;
	ID3D11RenderTargetView*	GetD3DRTView() const { return pRTView; }
};

typedef Ptr<CD3D11RenderTarget> PD3D11RenderTarget;

}

#endif
