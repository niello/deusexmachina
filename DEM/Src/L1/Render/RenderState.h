#pragma once
#ifndef __DEM_L1_RENDER_STATE_H__
#define __DEM_L1_RENDER_STATE_H__

#include <Core/Object.h>

// This class represents a state of the render device, which consists of the
// different settings configuring different pipeline stages. Internal
// representation of this object may differ significantly in different APIs,
// because they support different state settings. Independently of
// an exact state set, this class is designed for reuse of internal rendering
// API structures and for sorting by a render state during the rendering.
// State can define a state partially, but some APIs like DX10+ do not allow
// individual state changes. For that case, parent state must be provided on
// the creation time, and undefined values will be inherited from it. For each
// parent state, where any value in an atomic block is redefined, new child
// state must be created.

//???what with sampler states? completely different setting?
//each texture variable should store a pointer to the sampler state
//D3D9: texture variable is a texture-sampler pair, with any combinations
//two different samplers can sample one texture
//D3D11: ???

namespace Render
{

class CRenderState: public Core::CObject
{
public:

	// manager must have a routine CParams desc -> CStrID key

	// need unique key (to reuse already created, or store already created as chunks, if so, no need in a totally unique key)
	// need comparison function or operator, which compares by a key and by values not included to the key, for creation
	// need comparison by internal representation, for 2 already created state objects
	// need sorting by the key and non-keyed values like a depth bias, to minimize state changes

	// States supported by D3D11:
	// Rasterizer:
	//  - D3D11_FILL_MODE FillMode;
	//D3D11_CULL_MODE CullMode;
	//BOOL            FrontCounterClockwise;
	//INT             DepthBias;
	//FLOAT           DepthBiasClamp;
	//FLOAT           SlopeScaledDepthBias;
	//BOOL            DepthClipEnable;
	//BOOL            ScissorEnable;
	//BOOL            MultisampleEnable;
	//BOOL            AntialiasedLineEnable;
	// Depth-stencil:
	//BOOL                       DepthEnable;
	//D3D11_DEPTH_WRITE_MASK     DepthWriteMask;
	//D3D11_COMPARISON_FUNC      DepthFunc;
	//BOOL                       StencilEnable;
	//UINT8                      StencilReadMask;
	//UINT8                      StencilWriteMask;
	//D3D11_DEPTH_STENCILOP_DESC FrontFace;
	//D3D11_DEPTH_STENCILOP_DESC BackFace;
		//D3D11_STENCIL_OP      StencilFailOp;
		//D3D11_STENCIL_OP      StencilDepthFailOp;
		//D3D11_STENCIL_OP      StencilPassOp;
		//D3D11_COMPARISON_FUNC StencilFunc;
	// Blend:
	//BOOL                           AlphaToCoverageEnable;
	//BOOL                           IndependentBlendEnable;
	//D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
		//BOOL           BlendEnable;
		//D3D11_BLEND    SrcBlend;
		//D3D11_BLEND    DestBlend;
		//D3D11_BLEND_OP BlendOp;
		//D3D11_BLEND    SrcBlendAlpha;
		//D3D11_BLEND    DestBlendAlpha;
		//D3D11_BLEND_OP BlendOpAlpha;
		//UINT8          RenderTargetWriteMask;

	// States supported by D3D9:
	// FFP:
	//D3DRS_SHADEMODE                   = 9,
	//D3DRS_FOGENABLE                   = 28,
	//D3DRS_SPECULARENABLE              = 29,
	//D3DRS_FOGCOLOR                    = 34,
	//D3DRS_FOGTABLEMODE                = 35,
	//D3DRS_FOGSTART                    = 36,
	//D3DRS_FOGEND                      = 37,
	//D3DRS_FOGDENSITY                  = 38,
	//D3DRS_RANGEFOGENABLE              = 48,
	//D3DRS_LIGHTING                    = 137,
	//D3DRS_AMBIENT                     = 139,
	//D3DRS_FOGVERTEXMODE               = 140,
	//D3DRS_COLORVERTEX                 = 141,
	//D3DRS_LOCALVIEWER                 = 142,
	//D3DRS_NORMALIZENORMALS            = 143,
	//D3DRS_DIFFUSEMATERIALSOURCE       = 145,
	//D3DRS_SPECULARMATERIALSOURCE      = 146,
	//D3DRS_AMBIENTMATERIALSOURCE       = 147,
	//D3DRS_EMISSIVEMATERIALSOURCE      = 148,
	//D3DRS_VERTEXBLEND                 = 151,
	//D3DRS_INDEXEDVERTEXBLENDENABLE    = 167,
	//D3DRS_TWEENFACTOR                 = 170,
	//
	// D3D9-only:
	//D3DRS_SRGBWRITEENABLE             = 194,
	//D3DRS_ALPHATESTENABLE             = 15,
	//D3DRS_ALPHAREF                    = 24,
	//D3DRS_ALPHAFUNC                   = 25,
	//D3DRS_LASTPIXEL                   = 16,
	//D3DRS_DITHERENABLE                = 26,
	//D3DRS_WRAP0-15                    = 128,
	//D3DRS_CLIPPLANEENABLE             = 152,
	//D3DRS_POINTSIZE                   = 154,
	//D3DRS_POINTSIZE_MIN               = 155,
	//D3DRS_POINTSPRITEENABLE           = 156,
	//D3DRS_POINTSCALEENABLE            = 157,
	//D3DRS_POINTSCALE_A                = 158,
	//D3DRS_POINTSCALE_B                = 159,
	//D3DRS_POINTSCALE_C                = 160,
	//D3DRS_POINTSIZE_MAX               = 166,
	//D3DRS_PATCHEDGESTYLE              = 163,
	//D3DRS_DEBUGMONITORTOKEN           = 165,
	//D3DRS_POSITIONDEGREE              = 172,
	//D3DRS_NORMALDEGREE                = 173,
	//D3DRS_MINTESSELLATIONLEVEL        = 178,
	//D3DRS_MAXTESSELLATIONLEVEL        = 179,
	//D3DRS_ADAPTIVETESS_X              = 180,
	//D3DRS_ADAPTIVETESS_Y              = 181,
	//D3DRS_ADAPTIVETESS_Z              = 182,
	//D3DRS_ADAPTIVETESS_W              = 183,
	//D3DRS_ENABLEADAPTIVETESSELLATION  = 184,
	//
	// Rasterizer:
	//D3DRS_FILLMODE                    = 8,
	//D3DRS_CULLMODE                    = 22,
	//D3DRS_MULTISAMPLEANTIALIAS        = 161,
	//D3DRS_MULTISAMPLEMASK             = 162,
	//D3DRS_ANTIALIASEDLINEENABLE       = 176,
	//D3DRS_CLIPPING                    = 136, //???is as DX11 DepthClipEnable?
	//D3DRS_DEPTHBIAS                   = 195,
	//D3DRS_SLOPESCALEDEPTHBIAS         = 175,
	//D3DRS_SCISSORTESTENABLE           = 174,
	//
	// Depth-stencil:
	//D3DRS_ZENABLE                     = 7,	// bool + D3DZB_USEW
	//D3DRS_ZWRITEENABLE                = 14,
	//D3DRS_ZFUNC                       = 23,
	//D3DRS_STENCILENABLE               = 52,
	//D3DRS_STENCILFAIL                 = 53,
	//D3DRS_STENCILZFAIL                = 54,
	//D3DRS_STENCILPASS                 = 55,
	//D3DRS_STENCILFUNC                 = 56,
	//D3DRS_STENCILREF                  = 57,
	//D3DRS_STENCILMASK                 = 58,
	//D3DRS_STENCILWRITEMASK            = 59,
	//D3DRS_TWOSIDEDSTENCILMODE         = 185,
	//D3DRS_CCW_STENCILFAIL             = 186,
	//D3DRS_CCW_STENCILZFAIL            = 187,
	//D3DRS_CCW_STENCILPASS             = 188,
	//D3DRS_CCW_STENCILFUNC             = 189,
	//
	// Blend:
	//D3DRS_SRCBLEND                    = 19,
	//D3DRS_DESTBLEND                   = 20,
	//D3DRS_ALPHABLENDENABLE            = 27,
	//D3DRS_TEXTUREFACTOR               = 60, // Color for blending
	//D3DRS_BLENDOP                     = 171,
	//D3DRS_SEPARATEALPHABLENDENABLE    = 206,
	//D3DRS_SRCBLENDALPHA               = 207,
	//D3DRS_DESTBLENDALPHA              = 208,
	//D3DRS_BLENDOPALPHA                = 209
	//D3DRS_COLORWRITEENABLE            = 168,
	//D3DRS_COLORWRITEENABLE1           = 190,
	//D3DRS_COLORWRITEENABLE2           = 191,
	//D3DRS_COLORWRITEENABLE3           = 192,
	//D3DRS_BLENDFACTOR                 = 193,
//???
	//D3DTSS_COLOROP                = 1,
	//D3DTSS_COLORARG1              = 2,
	//D3DTSS_COLORARG2              = 3,
	//D3DTSS_ALPHAOP                = 4,
	//D3DTSS_ALPHAARG1              = 5,
	//D3DTSS_ALPHAARG2              = 6,
	//D3DTSS_BUMPENVMAT00           = 7,
	//D3DTSS_BUMPENVMAT01           = 8,
	//D3DTSS_BUMPENVMAT10           = 9,
	//D3DTSS_BUMPENVMAT11           = 10,
	//D3DTSS_TEXCOORDINDEX          = 11,
	//D3DTSS_BUMPENVLSCALE          = 22,
	//D3DTSS_BUMPENVLOFFSET         = 23,
	//D3DTSS_TEXTURETRANSFORMFLAGS  = 24,
	//D3DTSS_COLORARG0              = 26,
	//D3DTSS_ALPHAARG0              = 27,
	//D3DTSS_RESULTARG              = 28,
	//D3DTSS_CONSTANT               = 32,
};

};

#endif