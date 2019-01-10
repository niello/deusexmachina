#pragma once
#ifndef __DEM_L1_RENDER_D3D9_STATE_H__
#define __DEM_L1_RENDER_D3D9_STATE_H__

#include <Render/RenderState.h>

// Direct3D9 render state implementation
// NB: this implementation stores all the relevant D3D9 render states in each render state object.
// To save space, keys are stored only once in a static array D3DStates[], and values are stored per-object
// in D3DStateValues[] AT THE SAME indices. This is a key implementation detail. You should NEVER change
// an order of keys in D3DStates[], because all values are referenced by hardcoded indices instead of
// implementing a switch-case method like GetIndexForD3D9RenderStateKey().

typedef enum _D3DRENDERSTATETYPE D3DRENDERSTATETYPE;
typedef unsigned long DWORD;

//???!!! D3D9-only:
//D3DRS_SRGBWRITEENABLE             = 194,
//D3DRS_WRAP0-15                    = 128,
//D3DRS_POINTSIZE                   = 154,
//D3DRS_POINTSIZE_MIN               = 155,
//D3DRS_POINTSPRITEENABLE           = 156,
//D3DRS_POINTSIZE_MAX               = 166,

namespace Render
{
typedef Ptr<class CD3D9Shader> PD3D9Shader;

class CD3D9RenderState: public CRenderState
{
	__DeclareClass(CD3D9RenderState);

public:

	enum
	{
		// Rasterizer
		D3D9_FILLMODE = 0,
		D3D9_CULLMODE,
		D3D9_MULTISAMPLEANTIALIAS,
		D3D9_MULTISAMPLEMASK,
		D3D9_ANTIALIASEDLINEENABLE,
		D3D9_DEPTHBIAS,
		D3D9_SLOPESCALEDEPTHBIAS,
		D3D9_SCISSORTESTENABLE,

		// Depth-stencil
		D3D9_ZENABLE,
		D3D9_ZWRITEENABLE,
		D3D9_ZFUNC,
		D3D9_STENCILENABLE,
		D3D9_STENCILFAIL,
		D3D9_STENCILZFAIL,
		D3D9_STENCILPASS,
		D3D9_STENCILFUNC,
		D3D9_STENCILREF,
		D3D9_STENCILMASK,
		D3D9_STENCILWRITEMASK,
		D3D9_TWOSIDEDSTENCILMODE,
		D3D9_CCW_STENCILFAIL,
		D3D9_CCW_STENCILZFAIL,
		D3D9_CCW_STENCILPASS,
		D3D9_CCW_STENCILFUNC,

		// Blend
		D3D9_SRCBLEND,
		D3D9_DESTBLEND,
		D3D9_ALPHABLENDENABLE,
		D3D9_BLENDOP,
		D3D9_SEPARATEALPHABLENDENABLE,
		D3D9_SRCBLENDALPHA,
		D3D9_DESTBLENDALPHA,
		D3D9_BLENDOPALPHA,
		D3D9_COLORWRITEENABLE,
		//D3D9_COLORWRITEENABLE1,
		//D3D9_COLORWRITEENABLE2,
		//D3D9_COLORWRITEENABLE3,
		//D3D9_TEXTUREFACTOR, // Color for blending
		D3D9_BLENDFACTOR,

		//Misc
		D3D9_ALPHATESTENABLE,
		D3D9_ALPHAREF,
		D3D9_ALPHAFUNC,
		D3D9_CLIPPLANEENABLE,

		D3D9_RS_COUNT
	};

	static const D3DRENDERSTATETYPE D3DStates[D3D9_RS_COUNT];

	PD3D9Shader	VS;
	PD3D9Shader	PS;
	DWORD		D3DStateValues[D3D9_RS_COUNT];

	CD3D9RenderState();
	virtual ~CD3D9RenderState();
};

typedef Ptr<CD3D9RenderState> PD3D9RenderState;

}

#endif
