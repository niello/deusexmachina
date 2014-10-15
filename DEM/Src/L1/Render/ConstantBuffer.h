#pragma once
#ifndef __DEM_L1_RENDER_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_CONSTANT_BUFFER_H__

#include <Core/Object.h>
#include <Render/GPUResourceDefs.h>

// A hardware GPU buffer that contains shader uniform constants

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CConstantBuffer: public Core::CObject
{
protected:

	Data::CFlags	Access;
	PVertexLayout	Layout;
	DWORD			VtxCount;

public:

	CVertexBuffer(): VtxCount(0) {}
	virtual ~CVertexBuffer() { Destroy(); } //!!!???virtual destructor & virtual Destroy()?!

	virtual bool	Create(PVertexLayout VertexLayout, DWORD VertexCount, DWORD BufferAccess) = 0;
	virtual void	Destroy() = 0;				// GPU rsrc
	virtual void*	Map(EMapType MapType) = 0;	// Buffer
	virtual void	Unmap() = 0;				// Buffer

	Data::CFlags			GetAccess() const { return Access; }
	PVertexLayout			GetVertexLayout() const { return Layout; }
	DWORD					GetVertexCount() const { return VtxCount; }
	DWORD					GetSizeInBytes() const { return Layout.IsValid() ? Layout->GetVertexSize() * VtxCount : 0; }
	//virtual bool			IsValid() const = 0; //???or check VtxCount & Access are not 0?
};

typedef Ptr<CVertexBuffer> PVertexBuffer;

}

#endif
