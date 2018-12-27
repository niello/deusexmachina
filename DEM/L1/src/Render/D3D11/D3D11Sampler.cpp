#include "D3D11Sampler.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11Sampler, 'SAM1', Render::CSampler);

bool CD3D11Sampler::Create(ID3D11SamplerState* pSampler)
{
	if (!pSampler) FAIL;
	pD3DSampler = pSampler;
	OK;
}
//---------------------------------------------------------------------

void CD3D11Sampler::InternalDestroy()
{
	SAFE_RELEASE(pD3DSampler);
}
//---------------------------------------------------------------------

}