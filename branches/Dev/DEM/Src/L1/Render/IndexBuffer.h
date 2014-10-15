#pragma once
#ifndef __DEM_L1_RENDER_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_INDEX_BUFFER_H__

#include <Core/Object.h>
#include <Render/GPUResourceDefs.h>

// A hardware GPU buffer that stores indices in a corresponding array of vertices

namespace Render
{

//!!!GPU resource, Buffer, Resource!
class CIndexBuffer: public Core::CObject
{
public:

	enum EFormat
	{
		Index16 = 2,
		Index32 = 4
	};

protected:

	Data::CFlags	Access; //!!!can use as generic flags!
	EFormat			IdxFormat;
	DWORD			IdxCount;

	void InternalDestroy() { IdxCount = 0; Access.ClearAll(); }

public:

	CIndexBuffer(): IdxFormat(Index16), IdxCount(0) {}
	virtual ~CIndexBuffer() { }

	virtual bool	Create(EFormat IndexType, DWORD IndexCount, DWORD BufferAccess) = 0;
	virtual void	Destroy() { InternalDestroy(); }	// GPU rsrc
	virtual void*	Map(EMapType MapType) = 0;			// Buffer or GPU rsrc
	virtual void	Unmap() = 0;						// Buffer or GPU rsrc

	Data::CFlags	GetAccess() const { return Access; }
	EFormat			GetIndexType() const { return IdxFormat; }
	DWORD			GetIndexCount() const { return IdxCount; }
	DWORD			GetSizeInBytes() const { return IdxFormat * IdxCount; }
	//virtual bool			IsValid() const = 0; //???or check IdxCount & Access are not 0?
};

typedef Ptr<CIndexBuffer> PIndexBuffer;

}

#endif
