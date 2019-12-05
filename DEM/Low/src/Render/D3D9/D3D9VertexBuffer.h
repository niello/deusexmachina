#pragma once
#include <Render/VertexBuffer.h>

// Direct3D9 implementation of a vertex buffer

struct IDirect3DVertexBuffer9;
typedef unsigned int UINT;

namespace Render
{

class CD3D9VertexBuffer: public CVertexBuffer
{
	FACTORY_CLASS_DECL;

protected:

	IDirect3DVertexBuffer9*	pBuffer = nullptr;
	UINT					Usage = 0;
	//UPTR					LockCount = 0;

	void InternalDestroy();

public:

	virtual ~CD3D9VertexBuffer() { InternalDestroy(); }

	bool					Create(CVertexLayout& Layout, IDirect3DVertexBuffer9* pVB);
	virtual void			Destroy() { InternalDestroy(); CVertexBuffer::Destroy(); }

	IDirect3DVertexBuffer9*	GetD3DBuffer() const { return pBuffer; }
	UINT					GetD3DUsage() const { return Usage; }
};

typedef Ptr<CD3D9VertexBuffer> PD3D9VertexBuffer;

}
