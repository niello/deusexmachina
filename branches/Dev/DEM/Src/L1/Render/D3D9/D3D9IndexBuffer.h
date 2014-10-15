#pragma once
#ifndef __DEM_L1_RENDER_D3D9_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_INDEX_BUFFER_H__

#include <Render/IndexBuffer.h>
#include <Events/EventsFwd.h>

// Direct3D9 implementation of an index buffer

struct IDirect3DIndexBuffer9;

namespace Render
{

class CD3D9IndexBuffer: public CIndexBuffer
{
protected:

	IDirect3DIndexBuffer9*	pBuffer;
	DWORD					LockCount;

	void InternalDestroy();

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CD3D9IndexBuffer(): pBuffer(NULL), LockCount(0) {}
	virtual ~CD3D9IndexBuffer() { InternalDestroy(); }

	virtual bool	Create(EFormat IndexType, DWORD IndexCount, DWORD BufferAccess) = 0;
	virtual void	Destroy() { InternalDestroy(); CIndexBuffer::InternalDestroy(); }
	virtual void*	Map(EMapType MapType);
	virtual void	Unmap();

	bool					IsValid() const { return !!pBuffer; }
	IDirect3DIndexBuffer9*	GetD3DBuffer() const { return pBuffer; }
};

typedef Ptr<CD3D9IndexBuffer> PD3D9IndexBuffer;

}

#endif
