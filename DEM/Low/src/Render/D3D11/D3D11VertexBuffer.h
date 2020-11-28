#pragma once
#include <Render/VertexBuffer.h>

// Direct3D11 implementation of a vertex buffer

struct ID3D11Buffer;
enum D3D11_USAGE;

namespace Render
{

class CD3D11VertexBuffer: public CVertexBuffer
{
	FACTORY_CLASS_DECL;

protected:

	ID3D11Buffer*	pBuffer = nullptr;
	D3D11_USAGE		D3DUsage = {};

	void InternalDestroy();

public:

	virtual ~CD3D11VertexBuffer() { InternalDestroy(); }

	bool			Create(CVertexLayout& Layout, ID3D11Buffer* pVB);
	virtual void	Destroy() { InternalDestroy(); CVertexBuffer::Destroy(); }

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
	D3D11_USAGE		GetD3DUsage() const { return D3DUsage; }
};

typedef Ptr<CD3D11VertexBuffer> PD3D11VertexBuffer;

}
