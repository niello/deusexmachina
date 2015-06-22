#pragma once
#ifndef __DEM_L1_RENDER_D3D11_STATE_H__
#define __DEM_L1_RENDER_D3D11_STATE_H__

#include <Render/RenderState.h>

// Direct3D11 render state implementation

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;

namespace Render
{

class CD3D11RenderState: public CRenderState
{
	//__DeclareClass(CD3D11RenderState);

public:

	ID3D11RasterizerState*		pRState;
	ID3D11DepthStencilState*	pDSState;
	ID3D11BlendState*			pBState;

	CD3D11RenderState(): pRState(NULL), pDSState(NULL), pBState(NULL) {}
	virtual ~CD3D11RenderState();

	//can store sorting order here, set by device on a single off-line sorting, read by renderers on a fast state sorting
	//device can sort with a recursively mirrored pattern, for example, and fast sorting compares just one value (and pointer comparison beforehand)

	bool operator =(const CD3D11RenderState& Other) const { return pRState == Other.pRState && pDSState == Other.pDSState && pBState == Other.pBState; }
	bool operator !=(const CD3D11RenderState& Other) const { return pRState != Other.pRState || pDSState != Other.pDSState || pBState != Other.pBState; }
};

inline CD3D11RenderState::~CD3D11RenderState()
{
	//SAFE_RELEASE(pRState);
	//SAFE_RELEASE(pDSState);
	//SAFE_RELEASE(pBState);
}
//---------------------------------------------------------------------

}

#endif
