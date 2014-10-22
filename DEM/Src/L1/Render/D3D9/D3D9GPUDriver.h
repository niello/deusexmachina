#pragma once
#ifndef __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__

#include <Render/GPUDriver.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

// Direct3D9 GPU device driver

struct ID3DXEffectPool; // D3DX

namespace Render
{

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D9GPUDriver);

protected:

	D3DCAPS9			D3DCaps;
	IDirect3DDevice9*	pD3DDevice;
	ID3DXEffectPool*	pEffectPool;

	CD3D9GPUDriver() {}

	friend class CD3D9DriverFactory;

public:

	virtual ~CD3D9GPUDriver() { }

	virtual PVertexLayout	CreateVertexLayout(); // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer	CreateVertexBuffer();
	virtual PIndexBuffer	CreateIndexBuffer();

	IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
	ID3DXEffectPool*		GetD3DEffectPool() const { return pEffectPool; }
};

}

#endif
