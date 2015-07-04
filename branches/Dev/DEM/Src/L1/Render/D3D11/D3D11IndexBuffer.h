#pragma once
#ifndef __DEM_L1_RENDER_D3D11_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D11_INDEX_BUFFER_H__

#include <Render/IndexBuffer.h>

// Direct3D11 implementation of an index buffer

struct ID3D11Buffer;

namespace Render
{

class CD3D11IndexBuffer: public CIndexBuffer
{
	__DeclareClass(CD3D11IndexBuffer);

protected:

	ID3D11Buffer*	pBuffer;

	void InternalDestroy();

public:

	CD3D11IndexBuffer(): pBuffer(NULL) {}
	virtual ~CD3D11IndexBuffer() { InternalDestroy(); }

	bool			Create(EIndexType Type, ID3D11Buffer* pIB);
	virtual void	Destroy() { InternalDestroy(); CIndexBuffer::InternalDestroy(); }
	virtual bool	IsValid() const { return !!pBuffer; }

	ID3D11Buffer*	GetD3DBuffer() const { return pBuffer; }
};

typedef Ptr<CD3D11IndexBuffer> PD3D11IndexBuffer;

}

#endif
