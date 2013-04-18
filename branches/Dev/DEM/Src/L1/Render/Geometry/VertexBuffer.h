#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_VERTEX_BUFFER_H__

#include <Render/Geometry/VertexLayout.h>
#include <Render/GPUResourceDefs.h>
#include <Events/Events.h>

// Storage of geometry vertices

namespace Render
{

class CVertexBuffer: public Core::CRefCounted
{
protected:

	IDirect3DVertexBuffer9*	pBuffer;
	PVertexLayout			Layout;
	EUsage					Usage;
	ECPUAccess				Access;
	DWORD					Count;
	DWORD					LockCount;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CVertexBuffer(): pBuffer(NULL), Count(0), LockCount(0) {}
	~CVertexBuffer() { Destroy(); }

	bool	Create(PVertexLayout VertexLayout, DWORD VertexCount, EUsage BufferUsage, ECPUAccess BufferAccess);
	void	Destroy();
	void*	Map(EMapType MapType);
	void	Unmap();

	PVertexLayout			GetVertexLayout() const { return Layout; }
	DWORD					GetVertexCount() const { return Count; }
	EUsage					GetUsage() const { return Usage; }
	ECPUAccess				GetAccess() const { return Access; }
	DWORD					GetByteSize() const { return Layout.isvalid() ? Layout->GetVertexSize() * Count : 0; }
	bool					IsValid() const { return !!pBuffer; }
	IDirect3DVertexBuffer9*	GetD3DBuffer() const { return pBuffer; }
};

typedef Ptr<CVertexBuffer> PVertexBuffer;

}

#endif
