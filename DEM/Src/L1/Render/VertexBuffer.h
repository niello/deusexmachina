#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_VERTEX_BUFFER_H__

#include <Render/VertexLayout.h>
#include <Render/GPUResourceDefs.h>

// A hardware GPU buffer that stores vertices

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CVertexBuffer: public Core::CObject
{
protected:

	Data::CFlags	Access; //!!!can use as generic flags!
	PVertexLayout	Layout;
	DWORD			VtxCount;

	void InternalDestroy() { Layout = NULL; VtxCount = 0; Access.ClearAll(); }

public:

	CVertexBuffer(): VtxCount(0) {}
	virtual ~CVertexBuffer() { }

	virtual bool	Create(PVertexLayout VertexLayout, DWORD VertexCount, DWORD BufferAccess) = 0;
	virtual void	Destroy() { InternalDestroy(); }	// GPU rsrc
	virtual void*	Map(EMapType MapType) = 0;			// Buffer or GPU rsrc
	virtual void	Unmap() = 0;						// Buffer or GPU rsrc

	Data::CFlags	GetAccess() const { return Access; }
	PVertexLayout	GetVertexLayout() const { return Layout; }
	DWORD			GetVertexCount() const { return VtxCount; }
	DWORD			GetSizeInBytes() const { return Layout.IsValid() ? Layout->GetVertexSize() * VtxCount : 0; }
	//virtual bool	IsValid() const = 0; //???or check Layout & VtxCount & Access are not 0?
};

typedef Ptr<CVertexBuffer> PVertexBuffer;

}

#endif
