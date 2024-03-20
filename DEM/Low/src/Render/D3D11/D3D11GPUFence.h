#pragma once
#include <Render/GPUFence.h>

// Direct3D11 implementation of a GPU fence

struct ID3D11Query;
struct ID3D11DeviceContext;

namespace Render
{

class CD3D11GPUFence : public IGPUFence
{
protected:

	// TODO: also add an implementation with ID3D11Fence
	ID3D11Query*         _pQuery = nullptr;
	ID3D11DeviceContext* _pImmediateCtx = nullptr;

public:

	CD3D11GPUFence(ID3D11Query* pQuery);
	virtual ~CD3D11GPUFence() override;

	virtual bool IsSignaled() const override;
	virtual void Wait() override;

	auto GetD3DQuery() const { return _pQuery; }
};

using PD3D11GPUFence = Ptr<CD3D11GPUFence>;

}
