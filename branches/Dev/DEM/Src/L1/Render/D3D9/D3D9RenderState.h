#pragma once
#ifndef __DEM_L1_RENDER_D3D9_STATE_H__
#define __DEM_L1_RENDER_D3D9_STATE_H__

#include <Render/RenderState.h>

// Direct3D9 render state implementation

struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DStateBlock9;

namespace Render
{

class CD3D9RenderState: public CRenderState
{
	__DeclareClass(CD3D9RenderState);

public:

	//can store all shaders in one hash map URI -> ID3D11DeviceChild
	//or pass through resource manager!
	IDirect3DVertexShader9*		pVS;
	IDirect3DPixelShader9*		pPS;
	IDirect3DStateBlock9*		pState;
	//???store CD3D9RenderState* parent, as state blocks can be relative?

	CD3D9RenderState(): pVS(NULL), pPS(NULL), pState(NULL) {}
	virtual ~CD3D9RenderState();
};

}

#endif
