#pragma once
#ifndef __DEM_L1_RENDER_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_INDEX_BUFFER_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>

// A hardware GPU buffer that stores indices in a corresponding array of vertices

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CIndexBuffer: public Core::CObject
{
	//__DeclareClassNoFactory;

protected:

	EIndexType		IndexType;
	DWORD			IndexCount;
	Data::CFlags	Access;

	void InternalDestroy() { IndexCount = 0; Access.ClearAll(); }

public:

	CIndexBuffer(): IndexCount(0) {}
	virtual ~CIndexBuffer() { InternalDestroy(); }

	virtual void	Destroy() { InternalDestroy(); }
	virtual bool	IsValid() const = 0;

	Data::CFlags	GetAccess() const { return Access; }
	EIndexType		GetIndexType() const { return IndexType; }
	DWORD			GetIndexCount() const { return IndexCount; }
	DWORD			GetSizeInBytes() const { return IndexCount * (DWORD)IndexType; }
};

typedef Ptr<CIndexBuffer> PIndexBuffer;

}

#endif
