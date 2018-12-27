#pragma once
#ifndef __DEM_L1_RENDER_D3D9_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_VERTEX_BUFFER_H__

#include <Render/VertexBuffer.h>

// Direct3D9 implementation of a vertex buffer

struct IDirect3DVertexBuffer9;
typedef unsigned int UINT;

namespace Render
{

class CD3D9VertexBuffer: public CVertexBuffer
{
	__DeclareClass(CD3D9VertexBuffer);

protected:

	IDirect3DVertexBuffer9*	pBuffer;
	UINT					Usage;
	//UPTR					LockCount;

	void InternalDestroy();

public:

	CD3D9VertexBuffer(): pBuffer(NULL)/*, LockCount(0)*/ {}
	virtual ~CD3D9VertexBuffer() { InternalDestroy(); }

	bool					Create(CVertexLayout& Layout, IDirect3DVertexBuffer9* pVB);
	virtual void			Destroy() { InternalDestroy(); CVertexBuffer::Destroy(); }

	IDirect3DVertexBuffer9*	GetD3DBuffer() const { return pBuffer; }
	UINT					GetD3DUsage() const { return Usage; }
};

typedef Ptr<CD3D9VertexBuffer> PD3D9VertexBuffer;

}

#endif
