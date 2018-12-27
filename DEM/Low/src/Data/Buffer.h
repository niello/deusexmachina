#pragma once
#ifndef __DEM_L1_BUFFER_H__
#define __DEM_L1_BUFFER_H__

// The CBuffer class encapsulates a chunk of raw memory into a C++ object which can be copied, compared and hashed.
// Based on mangalore Util::Blob_(C) 2006 Radon Labs GmbH

#include <Data/Type.h>
#include <Data/Hash.h>
#include <memory.h>

namespace Data
{

class CBuffer
{
protected:

	char*	pData;
	UPTR	DataSize;
	UPTR	Allocated;

	void	Allocate(UPTR Size);
	int		BinaryCompare(const CBuffer& Other) const;

public:

	CBuffer(): pData(NULL), DataSize(0), Allocated(0) {}
	CBuffer(const void* pSrc, UPTR SrcSize): pData(NULL), DataSize(0), Allocated(0) { Set(pSrc, SrcSize); }
	CBuffer(UPTR Size): pData(NULL), DataSize(0), Allocated(0) { Allocate(Size); }
	CBuffer(const CBuffer& Other): pData(NULL), DataSize(0), Allocated(0) { Set(Other.pData, Other.DataSize); }
	~CBuffer() { if (pData) n_free(pData); }

	void	Reserve(UPTR Size);
	void	Clear();
	void	Trim(UPTR Size) { n_assert(Size <= DataSize); DataSize = Size; }
	int		HashCode() const { Hash(pData, DataSize); }

	bool	IsValid() const { return !!pData; }
	bool	IsEmpty() const { return !pData || !DataSize; }
	void*	GetPtr() const { return pData; }
	int		GetSize() const { return DataSize; }
	int		GetCapacity() const { return Allocated; }
	void	Set(const void* pSrc, UPTR SrcSize);
	void	Read(char* pDst, UPTR StartIdx, UPTR EndIdx);
	void	Write(const char* pSrc, UPTR StartIdx, UPTR EndIdx);

	void operator =(const CBuffer& Other) { Set(Other.pData, Other.DataSize); }
	bool operator ==(const CBuffer& Other) const { return !BinaryCompare(Other); }
	bool operator !=(const CBuffer& Other) const { return !!BinaryCompare(Other); }
	bool operator >(const CBuffer& Other) const { return BinaryCompare(Other) > 0; }
	bool operator <(const CBuffer& Other) const { return BinaryCompare(Other) < 0; }
	bool operator >=(const CBuffer& Other) const { return BinaryCompare(Other) >= 0; }
	bool operator <=(const CBuffer& Other) const { return BinaryCompare(Other) <= 0; }
};

inline void CBuffer::Allocate(UPTR Size)
{
	n_assert(!IsValid());
	pData = (char*)n_malloc(Size);
	Allocated = Size;
	DataSize = Size;
}
//---------------------------------------------------------------------

inline void CBuffer::Clear()
{
	if (IsValid())
	{
		n_free(pData);
		pData = NULL;
		DataSize = 0;
		Allocated = 0;
	}
}
//---------------------------------------------------------------------

inline void CBuffer::Set(const void* pSrc, UPTR SrcSize)
{
	n_assert(pSrc || !SrcSize);

	if (!pData || Allocated < SrcSize)
	{
		n_free(pData);
		pData = (char*)n_malloc(SrcSize);
		Allocated = SrcSize;
	}
	DataSize = SrcSize;
	if (SrcSize) memcpy(pData, pSrc, SrcSize);
}
//---------------------------------------------------------------------

inline void CBuffer::Read(char* pDst, UPTR StartIdx, UPTR EndIdx)
{
	n_assert(pDst &&
		StartIdx < Allocated &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pDst, pData + StartIdx, EndIdx - StartIdx + 1);
}
//---------------------------------------------------------------------

inline void CBuffer::Write(const char* pSrc, UPTR StartIdx, UPTR EndIdx)
{
	n_assert(pSrc &&
		StartIdx < Allocated &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pData + StartIdx, pSrc, EndIdx - StartIdx + 1);
	if (DataSize < EndIdx) DataSize = EndIdx + 1;
}
//---------------------------------------------------------------------

inline void CBuffer::Reserve(UPTR Size)
{
	if (Allocated < Size)
	{
		Clear();
		Allocate(Size);
	}
	DataSize = Size;
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(Data::CBuffer, 9)
#define TBuffer DATA_TYPE(Data::CBuffer)

#endif