#pragma once
#ifndef __DEM_L1_RENDER_VERTEX_BUFFER_H__
#define __DEM_L1_RENDER_VERTEX_BUFFER_H__

#include <Render/VertexLayout.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// A hardware GPU buffer that stores vertices

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CVertexBuffer: public Core::CObject
{
	//__DeclareClassNoFactory;

protected:

	PVertexLayout	VertexLayout;
	UPTR			VertexCount;
	Data::CFlags	Access;

	void InternalDestroy() { VertexLayout = NULL; VertexCount = 0; Access.ClearAll(); }

public:

	CVertexBuffer(): VertexCount(0) {}
	virtual ~CVertexBuffer() { InternalDestroy(); }

	virtual void	Destroy() { InternalDestroy(); }

	CVertexLayout*	GetVertexLayout() const { return VertexLayout.GetUnsafe(); }
	UPTR			GetVertexCount() const { return VertexCount; }
	Data::CFlags	GetAccess() const { return Access; }
	UPTR			GetSizeInBytes() const { return VertexLayout.IsValidPtr() ? VertexLayout->GetVertexSizeInBytes() * VertexCount : 0; }
	bool			IsValid() const { VertexLayout.IsValidPtr(); }
};

typedef Ptr<CVertexBuffer> PVertexBuffer;

}

#endif
