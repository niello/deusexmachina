#pragma once
#ifndef __DEM_L1_RENDER_D3D11_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_VERTEX_BUFFER_H__

#include <Render/VertexBuffer.h>

// Direct3D11 implementation of a vertex buffer

struct ID3D11Buffer;
enum D3D11_USAGE;

namespace Render
{

class CD3D11VertexBuffer: public CVertexBuffer
{
	__DeclareClass(CD3D11VertexBuffer);

protected:

	ID3D11Buffer*	pBuffer;
	D3D11_USAGE		D3DUsage;

	void InternalDestroy();

public:

	CD3D11VertexBuffer(): pBuffer(NULL) {}
	virtual ~CD3D11VertexBuffer() { InternalDestroy(); }

	bool			Create(CVertexLayout& Layout, ID3D11Buffer* pVB);
	virtual void	Destroy() { InternalDestroy(); CVertexBuffer::InternalDestroy(); }

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
	D3D11_USAGE		GetD3DUsage() const { return D3DUsage; }
};

typedef Ptr<CD3D11VertexBuffer> PD3D11VertexBuffer;

}

#endif
