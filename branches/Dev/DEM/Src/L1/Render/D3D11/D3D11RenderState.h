#pragma once
#ifndef __DEM_L1_RENDER_D3D11_STATE_H__
#define __DEM_L1_RENDER_D3D11_STATE_H__

#include <Render/RenderState.h>

// Direct3D11 render state implementation

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;

namespace Render
{

class CD3D11RenderState: public CRenderState
{
	__DeclareClass(CD3D11RenderState);

public:

	ID3D11VertexShader*			pVS;
	ID3D11HullShader*			pHS;
	ID3D11DomainShader*			pDS;
	ID3D11GeometryShader*		pGS;
	ID3D11PixelShader*			pPS;
	ID3D11RasterizerState*		pRState;
	ID3D11DepthStencilState*	pDSState;
	ID3D11BlendState*			pBState;

	CD3D11RenderState(): pVS(NULL), pHS(NULL), pDS(NULL), pGS(NULL), pPS(NULL), pRState(NULL), pDSState(NULL), pBState(NULL) {}
	virtual ~CD3D11RenderState();
};

}

#endif
