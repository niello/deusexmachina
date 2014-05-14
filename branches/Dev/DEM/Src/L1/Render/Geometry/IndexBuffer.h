#pragma once
#ifndef __DEM_L1_RENDER_INDEX_BUFFER_H__
#define __DEM_L1_RENDER_INDEX_BUFFER_H__

#include <Core/Object.h>
#include <Render/GPUResourceDefs.h>
#include <Render/D3D9Fwd.h>
#include <Events/EventsFwd.h>

// Storage of geometry index array for corresponding vertex array.

namespace Render
{

class CIndexBuffer: public Core::CObject
{
public:

	enum EType
	{
		Index16,
		Index32
	};

protected:

	IDirect3DIndexBuffer9*	pBuffer;
	EType					Type;
	EUsage					Usage;
	ECPUAccess				Access;
	DWORD					Count;
	DWORD					LockCount;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);

public:

	CIndexBuffer(): pBuffer(NULL), Type(Index16), Count(0), LockCount(0) {}
	~CIndexBuffer() { Destroy(); }

	bool	Create(EType IndexType, DWORD IndexCount, EUsage BufferUsage, ECPUAccess BufferAccess);
	void	Destroy();
	void*	Map(EMapType MapType);
	void	Unmap();

	DWORD					GetIndexCount() const { return Count; }
	EType					GetIndexType() const { return Type; }
	EUsage					GetUsage() const { return Usage; }
	ECPUAccess				GetAccess() const { return Access; }
	IDirect3DIndexBuffer9*	GetD3DBuffer() const { return pBuffer; }
};

typedef Ptr<CIndexBuffer> PIndexBuffer;

}

#endif
