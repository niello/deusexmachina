#pragma once
#ifndef __DEM_L1_RENDER_D3D11_DEPTH_STENCIL_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_DEPTH_STENCIL_BUFFER_H__

#include <Render/DepthStencilBuffer.h>

// D3D11 implementation of depth-stencil buffer

struct ID3D11DepthStencilView;

namespace Render
{

class CD3D11DepthStencilBuffer: public CDepthStencilBuffer
{
	__DeclareClass(CD3D11DepthStencilBuffer);

protected:

	ID3D11DepthStencilView* pDSView;

public:

	CD3D11DepthStencilBuffer(): pDSView(NULL) {}

	bool					Create(ID3D11DepthStencilView* pDSV); // For internal use
	virtual void			Destroy();
	virtual bool			IsValid() const { return !!pDSView; }
	ID3D11DepthStencilView*	GetD3DDSView() const { return pDSView; }
};

typedef Ptr<CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;

}

#endif
