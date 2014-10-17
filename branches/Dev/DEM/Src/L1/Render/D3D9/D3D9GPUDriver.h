#pragma once
#ifndef __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__

#include <Render/GPUDriver.h>

// Direct3D9 GPU device driver

namespace Render
{

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D9GPUDriver);

protected:

public:

	CD3D9GPUDriver() {}
	virtual ~CD3D9GPUDriver() { }

	virtual CVertexLayout*	CreateVertexLayout(); // Prefer GetVertexLayout() when possible
	virtual CVertexBuffer*	CreateVertexBuffer();
	virtual CIndexBuffer*	CreateIndexBuffer();
};

}

#endif
