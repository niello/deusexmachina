#pragma once
#ifndef __DEM_L1_RENDER_D3D9_STATE_H__
#define __DEM_L1_RENDER_D3D9_STATE_H__

#include <Render/RenderState.h>

// Direct3D9 render state implementation

struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

namespace Render
{

class CD3D9RenderState: public CRenderState
{
	__DeclareClass(CD3D9RenderState);

public:

	IDirect3DVertexShader9*	pVS;
	IDirect3DPixelShader9*	pPS;

	//???store full desc here in key-value pairs, use only leaves and set RS by diff?!
	//can choose supported RS and setup O(1) index mapping to a key-value array
	//can store static array of Idx -> RS, so can store keys only once and not per CRenderState
	//RS -> Idx by switch-case function

	CD3D9RenderState(): pVS(NULL), pPS(NULL) {}
	virtual ~CD3D9RenderState();
};

typedef Ptr<CD3D9RenderState> PD3D9RenderState;

}

#endif
