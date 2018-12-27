#include "D3D9Sampler.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9Sampler, 'SAM9', Render::CSampler);

const D3DSAMPLERSTATETYPE CD3D9Sampler::D3DStates[D3D9_SS_COUNT] =
{
	D3DSAMP_ADDRESSU,
	D3DSAMP_ADDRESSV,
	D3DSAMP_ADDRESSW,
	D3DSAMP_BORDERCOLOR,
	D3DSAMP_MAGFILTER,
	D3DSAMP_MINFILTER,
	D3DSAMP_MIPFILTER,
	D3DSAMP_MIPMAPLODBIAS,
	D3DSAMP_MAXMIPLEVEL,
	D3DSAMP_MAXANISOTROPY
	//D3DSAMP_SRGBTEXTURE,
	//D3DSAMP_ELEMENTINDEX
};
//---------------------------------------------------------------------

}