#pragma once
#ifndef __DEM_L1_RENDER_D3D11_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_INDEX_BUFFER_H__

#include <Render/IndexBuffer.h>

// Direct3D11 implementation of an index buffer

struct ID3D11Buffer;
enum D3D11_USAGE;

namespace Render
{

class CD3D11IndexBuffer: public CIndexBuffer
{
	__DeclareClass(CD3D11IndexBuffer);

protected:

	ID3D11Buffer*	pBuffer;
	D3D11_USAGE		D3DUsage;

	void InternalDestroy();

public:

	CD3D11IndexBuffer(): pBuffer(NULL) {}
	virtual ~CD3D11IndexBuffer() { InternalDestroy(); }

	bool			Create(EIndexType Type, ID3D11Buffer* pIB);
	virtual void	Destroy() { InternalDestroy(); CIndexBuffer::Destroy(); }
	virtual bool	IsValid() const { return !!pBuffer; }

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
	D3D11_USAGE		GetD3DUsage() const { return D3DUsage; }
};

typedef Ptr<CD3D11IndexBuffer> PD3D11IndexBuffer;

}

#endif
