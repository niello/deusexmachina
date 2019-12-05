#pragma once
#include <Render/Sampler.h>

// A Direct3D11 implementation of a sampler state object

struct ID3D11SamplerState;

namespace Render
{

class CD3D11Sampler: public CSampler
{
	FACTORY_CLASS_DECL;

protected:

	ID3D11SamplerState* pD3DSampler = nullptr;

	void				InternalDestroy();

public:

	virtual ~CD3D11Sampler() { InternalDestroy(); }

	bool				Create(ID3D11SamplerState* pSampler); 
	virtual void		Destroy() { InternalDestroy(); }

	ID3D11SamplerState*	GetD3DSampler() const { return pD3DSampler; }
};

typedef Ptr<CD3D11Sampler> PD3D11Sampler;

}
