#pragma once
#ifndef __DEM_L1_RENDER_D3D9_SAMPLER_H__
#define __DEM_L1_RENDER_D3D9_SAMPLER_H__

#include <Render/Sampler.h>

// A Direct3D9 implementation of a sampler state object
// NB: this implementation stores all the relevant D3D9 sampler states in each sampler object.
// To save space, keys are stored only once in a static array D3DStates[], and values are stored per-object
// in D3DStateValues[] AT THE SAME indices. This is a key implementation detail. You should NEVER change
// an order of keys in D3DStates[], because all values are referenced by hardcoded indices instead of
// implementing a switch-case method like GetIndexForD3D9SamplerStateKey().

typedef enum _D3DSAMPLERSTATETYPE D3DSAMPLERSTATETYPE;
typedef unsigned long DWORD;

namespace Render
{

class CD3D9Sampler: public CSampler
{
	__DeclareClass(CD3D9Sampler);

public:

	enum
	{
		D3D9_ADDRESSU = 0,
		D3D9_ADDRESSV,
		D3D9_ADDRESSW,
		D3D9_BORDERCOLOR,
		D3D9_MAGFILTER,
		D3D9_MINFILTER,
		D3D9_MIPFILTER,
		D3D9_MIPMAPLODBIAS,
		D3D9_MAXMIPLEVEL,
		D3D9_MAXANISOTROPY,
		//D3D9_SRGBTEXTURE,
		//D3D9_ELEMENTINDEX, // for multielement textures, default 0

		D3D9_SS_COUNT
	};

	static const D3DSAMPLERSTATETYPE D3DStates[D3D9_SS_COUNT];

	DWORD D3DStateValues[D3D9_SS_COUNT];
};

typedef Ptr<CD3D9Sampler> PD3D9Sampler;

}

#endif
