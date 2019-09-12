#pragma once
#ifndef __DEM_L1_RENDER_D3D11_SAMPLER_H__
#define __DEM_L1_RENDER_D3D11_SAMPLER_H__

#include <Render/Sampler.h>

// A Direct3D11 implementation of a sampler state object

struct ID3D11SamplerState;

namespace Render
{

class CD3D11Sampler: public CSampler
{
	__DeclareClass(CD3D11Sampler);

protected:

	ID3D11SamplerState* pD3DSampler;

	void				InternalDestroy();

public:

	CD3D11Sampler(): pD3DSampler(nullptr) {}
	virtual ~CD3D11Sampler() { InternalDestroy(); }

	bool				Create(ID3D11SamplerState* pSampler); 
	virtual void		Destroy() { InternalDestroy(); }

	ID3D11SamplerState*	GetD3DSampler() const { return pD3DSampler; }
};

typedef Ptr<CD3D11Sampler> PD3D11Sampler;

}

#endif
