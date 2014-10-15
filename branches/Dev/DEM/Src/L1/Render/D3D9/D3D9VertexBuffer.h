#pragma once
#ifndef __DEM_L1_RENDER_D3D9_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_VERTEX_BUFFER_H__

#include <Render/VertexBuffer.h>
#include <Events/EventsFwd.h>

// Direct3D9 implementation of a vertex buffer

struct IDirect3DVertexBuffer9;

namespace Render
{

class CD3D9VertexBuffer: public CVertexBuffer
{
protected:

	IDirect3DVertexBuffer9*	pBuffer;
	DWORD					LockCount;

	void InternalDestroy();

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CD3D9VertexBuffer(): pBuffer(NULL), LockCount(0) {}
	virtual ~CD3D9VertexBuffer() { InternalDestroy(); }

	virtual bool	Create(PVertexLayout VertexLayout, DWORD VertexCount, DWORD BufferAccess);
	virtual void	Destroy() { InternalDestroy(); CVertexBuffer::InternalDestroy(); }
	virtual void*	Map(EMapType MapType);
	virtual void	Unmap();

	bool					IsValid() const { return !!pBuffer; }
	IDirect3DVertexBuffer9*	GetD3DBuffer() const { return pBuffer; }
};

typedef Ptr<CD3D9VertexBuffer> PD3D9VertexBuffer;

}

#endif
