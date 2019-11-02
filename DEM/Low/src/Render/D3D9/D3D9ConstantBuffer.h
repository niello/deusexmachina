#pragma once
#include <Render/ConstantBuffer.h>
#include <Render/RenderFwd.h>
#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Data/Flags.h>

// A Direct3D9 implementation of a shader constant buffer.

typedef int BOOL;

namespace Render
{

class CD3D9ConstantBuffer: public CConstantBuffer
{
	__DeclareClassNoFactory;

protected:

	enum
	{
		CB9_DirtyFloat4	= 0x01,
		CB9_DirtyInt4	= 0x02,
		CB9_DirtyBool	= 0x04,
		CB9_Temporary	= 0x08,
		CB9_InWriteMode	= 0x10,

		CB9_AnyDirty = (CB9_DirtyFloat4 | CB9_DirtyInt4 | CB9_DirtyBool)
	};

	float*			pFloat4Data = nullptr; //???or one ptr and offsets ??on the fly?? ?
	UPTR			Float4Count = 0;

	int*			pInt4Data = nullptr;
	UPTR			Int4Count = 0;

	BOOL*			pBoolData = nullptr;
	UPTR			BoolCount = 0;

	Data::CFlags	Flags;

	// D3D9 pseudo-buffers are too tightly coupled with shader metadata to be reused with another metadata
	PSM30ConstantBufferParam _Meta;

public:

	CD3D9ConstantBuffer(PSM30ConstantBufferParam Meta, const CD3D9ConstantBuffer* pInitData, bool Temporary = false);
	virtual ~CD3D9ConstantBuffer() override;

	virtual bool IsValid() const override { return pFloat4Data || pInt4Data || pBoolData; }
	virtual bool IsInWriteMode() const override { return Flags.Is(CB9_InWriteMode); }
	virtual bool IsDirty() const override { return Flags.IsAny(CB9_AnyDirty); }
	virtual bool IsTemporary() const override { return Flags.Is(CB9_Temporary); }
	virtual U8   GetAccessFlags() const override;

	//!!!for IConst can compute universal offset! anyway need to set proper dirty flag (can deduce from offset btw)!
	void         WriteData(ESM30RegisterSet RegSet, UPTR OffsetInBytes, const void* pData, UPTR Size);
	bool         IsDirtyFloat4() const { return Flags.Is(CB9_DirtyFloat4); }
	bool         IsDirtyInt4() const { return Flags.Is(CB9_DirtyInt4); }
	bool         IsDirtyBool() const { return Flags.Is(CB9_DirtyBool); }
	const auto*  GetMetadata() const { return _Meta.Get(); }
	const float* GetFloat4Data() const { return pFloat4Data; }
	const int*   GetInt4Data() const { return pInt4Data; }
	const BOOL*  GetBoolData() const { return pBoolData; }

	void         OnBegin() { Flags.Set(CB9_InWriteMode); }	// For internal use by the GPUDriver
	void         OnCommit() { Flags.Clear(CB9_AnyDirty | CB9_InWriteMode); }	// For internal use by the GPUDriver
};

typedef Ptr<CD3D9ConstantBuffer> PD3D9ConstantBuffer;

}
