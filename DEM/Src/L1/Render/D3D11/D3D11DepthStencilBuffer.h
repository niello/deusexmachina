#pragma once
#ifndef __DEM_L1_RENDER_D3D11_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_DEPTH_STENCIL_BUFFER_H__

#include <Render/DepthStencilBuffer.h>

// D3D11 implementation of depth-stencil buffer

struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;

namespace Render
{
typedef Ptr<class CD3D11Texture> PD3D11Texture;

class CD3D11DepthStencilBuffer: public CDepthStencilBuffer
{
	__DeclareClass(CD3D11DepthStencilBuffer);

protected:

	ID3D11DepthStencilView* pDSView;
	PD3D11Texture			Texture;

	void					InternalDestroy();

public:

	CD3D11DepthStencilBuffer(): pDSView(NULL) {}
	virtual ~CD3D11DepthStencilBuffer() { InternalDestroy(); }

	bool					Create(ID3D11DepthStencilView* pDSV, ID3D11ShaderResourceView* pSRV); // For internal use
	virtual void			Destroy() { InternalDestroy(); }
	virtual bool			IsValid() const { return !!pDSView; }
	virtual CTexture*		GetShaderResource() const;
	ID3D11DepthStencilView*	GetD3DDSView() const { return pDSView; }
};

typedef Ptr<CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;

}

#endif
