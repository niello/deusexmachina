#pragma once
#ifndef __DEM_L1_RENDER_D3D11_STATE_H__
#define __DEM_L1_RENDER_D3D11_STATE_H__

#include <Render/RenderState.h>
#include <Data/StringID.h>

// Direct3D11 render state implementation

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;

namespace Render
{
typedef Ptr<class CD3D11Shader> PD3D11Shader;

class CD3D11RenderState: public CRenderState
{
	__DeclareClass(CD3D11RenderState);

public:

	CStrID						InputSigID;
	PD3D11Shader				VS;
	PD3D11Shader				HS;
	PD3D11Shader				DS;
	PD3D11Shader				GS;
	PD3D11Shader				PS;
	ID3D11RasterizerState*		pRState;
	ID3D11DepthStencilState*	pDSState;
	ID3D11BlendState*			pBState;
	unsigned int				StencilRef; //???get from stored desc?
	float						BlendFactorRGBA[4]; //???get from stored desc?
	unsigned int				SampleMask; //???get from stored desc?

	CD3D11RenderState(): pRState(NULL), pDSState(NULL), pBState(NULL) {}
	virtual ~CD3D11RenderState();
};

typedef Ptr<CD3D11RenderState> PD3D11RenderState;

}

#endif
