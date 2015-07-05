#pragma once
#ifndef __DEM_L1_RENDER_D3D9_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_INDEX_BUFFER_H__

#include <Render/IndexBuffer.h>

// Direct3D9 implementation of an index buffer

struct IDirect3DIndexBuffer9;

namespace Render
{

class CD3D9IndexBuffer: public CIndexBuffer
{
	__DeclareClass(CD3D9IndexBuffer);

protected:

	IDirect3DIndexBuffer9*	pBuffer;
	UINT					Usage;
	DWORD					LockCount;

	void InternalDestroy();

public:

	CD3D9IndexBuffer(): pBuffer(NULL), LockCount(0) {}
	virtual ~CD3D9IndexBuffer() { InternalDestroy(); }

	bool					Create(EIndexType Type, IDirect3DIndexBuffer9* pIB);
	virtual void			Destroy() { InternalDestroy(); CIndexBuffer::InternalDestroy(); }
	virtual bool			IsValid() const { return !!pBuffer; }

	IDirect3DIndexBuffer9*	GetD3DBuffer() const { return pBuffer; }
	UINT					GetD3DUsage() const { return Usage; }
};

typedef Ptr<CD3D9IndexBuffer> PD3D9IndexBuffer;

}

#endif
