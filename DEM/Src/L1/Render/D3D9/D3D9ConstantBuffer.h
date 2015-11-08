#pragma once
#ifndef __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__
#define __DEM_L1_RENDER_D3D9_CONSTANT_BUFFER_H__

#include <Render/ConstantBuffer.h>
#include <Render/RenderFwd.h>
#include <Render/D3D9/D3D9Fwd.h>
#include <Data/Flags.h>

// A Direct3D9 implementation of a shader constant buffer.

namespace Render
{

class CD3D9ConstantBuffer: public CConstantBuffer
{
	__DeclareClass(CD3D9ConstantBuffer);

protected:

	enum
	{
		CB9_DirtyFloat4	= 0x01,
		CB9_DirtyInt4	= 0x02,
		CB9_DirtyBool	= 0x04
	};

	float*			pFloat4Data; //???or one ptr and offsets ??on the fly?? ?
	UPTR			Float4Count;

	int*			pInt4Data;
	UPTR			Int4Count;

	BOOL*			pBoolData;
	UPTR			BoolCount;

	HConstBuffer	Handle;		//!!!strictly bounds D3D9 CB to the host Shader! may copy relevant metadata instead, if inacceptable
	Data::CFlags	DirtyFlags;

	void InternalDestroy();

public:

	CD3D9ConstantBuffer(): pFloat4Data(NULL), pInt4Data(NULL), pBoolData(NULL), Float4Count(0), Int4Count(0), BoolCount(0), Handle(INVALID_HANDLE) {}
	virtual ~CD3D9ConstantBuffer() { InternalDestroy(); }

	bool			Create(const CD3D9ShaderBufferMeta& Meta);
	virtual void	Destroy() { InternalDestroy(); /*CConstantBuffer::Destroy();*/ }
	virtual bool	IsValid() const { return pFloat4Data || pInt4Data || pBoolData; }

	//!!!for IConst can compute universal offset! anyway need to set proper dirty flag (can deduce from offset btw)!
	void			WriteData(ED3D9ShaderRegisterSet RegSet, UPTR Offset, const void* pData, UPTR Size);
	bool			IsDirty() const { return DirtyFlags.IsAny(); }
	bool			IsDirtyFloat4() const { return DirtyFlags.Is(CB9_DirtyFloat4); }
	bool			IsDirtyInt4() const { return DirtyFlags.Is(CB9_DirtyInt4); }
	bool			IsDirtyBool() const { return DirtyFlags.Is(CB9_DirtyBool); }
	HConstBuffer	GetHandle() const { return Handle; }

	void			OnCommit() { DirtyFlags.ClearAll(); }	// For internal use by the GPUDriver
};

typedef Ptr<CD3D9ConstantBuffer> PD3D9ConstantBuffer;

inline void CD3D9ConstantBuffer::WriteData(ED3D9ShaderRegisterSet RegSet, UPTR Offset, const void* pData, UPTR Size)
{
	n_assert_dbg(pData && Size);

	char* pDest;
	UPTR Flag;
	switch (RegSet)
	{
		case Reg_Float4:
			pDest = (char*)pFloat4Data;
			Offset *= (sizeof(float) * 4);
			Flag = CB9_DirtyFloat4;
			break;
		case Reg_Int4:
			pDest = (char*)pInt4Data;
			Offset *= (sizeof(int) * 4);
			Flag = CB9_DirtyInt4;
			break;
		case Reg_Bool:
			pDest = (char*)pBoolData;
			Offset *= sizeof(BOOL);
			Flag = CB9_DirtyBool;
			break;
		default:
			pDest = NULL;
			break;
	};

	n_assert_dbg(pDest);

	//!!!???PERF:?!
	//if (memcmp(pMapped + Offset, pData, Size) == 0) return;
	memcpy(pDest + Offset, pData, Size);
	DirtyFlags.Set(Flag);
}
//---------------------------------------------------------------------

}

#endif
