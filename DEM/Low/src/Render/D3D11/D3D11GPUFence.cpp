#include "D3D11GPUFence.h"

#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{

CD3D11GPUFence::CD3D11GPUFence(ID3D11Query* pQuery)
	: _pQuery(pQuery)
{
	n_assert(_pQuery);

	ID3D11Device* pDevice = nullptr;
	_pQuery->GetDevice(&pDevice);

	// Only an immediate context allows GetData
	pDevice->GetImmediateContext(&_pImmediateCtx);
	n_assert(_pImmediateCtx);

	pDevice->Release();
}
//---------------------------------------------------------------------

CD3D11GPUFence::~CD3D11GPUFence()
{
	SAFE_RELEASE(_pQuery);
	SAFE_RELEASE(_pImmediateCtx);
}
//---------------------------------------------------------------------

bool CD3D11GPUFence::IsSignaled() const
{
	return _pImmediateCtx->GetData(_pQuery, nullptr, 0, 0) == S_OK; // D3D11_ASYNC_GETDATA_DONOTFLUSH ?
}
//---------------------------------------------------------------------

void CD3D11GPUFence::Wait()
{
	ZoneScoped;

	while (_pImmediateCtx->GetData(_pQuery, nullptr, 0, 0) != S_OK)
		std::this_thread::yield();
}
//---------------------------------------------------------------------

}
