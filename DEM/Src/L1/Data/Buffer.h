#pragma once
#ifndef __DEM_L1_BUFFER_H__
#define __DEM_L1_BUFFER_H__

// The CBuffer class encapsulates a chunk of raw memory into a C++ object which can be copied, compared and hashed.
// Based on mangalore Util::Blob_(C) 2006 Radon Labs GmbH

#include <kernel/ntypes.h>
#include <util/Hash.h>
#include <memory.h>
#include "Type.h"

namespace Data
{

class CBuffer
{
protected:

	char*	pData;
	int		DataSize;
	int		Allocated;

	void	Allocate(int DataSize);
	int		BinaryCompare(const CBuffer& Other) const;

public:

	CBuffer(): pData(NULL), DataSize(0), Allocated(0) {}
	CBuffer(const void* pSrc, int SrcSize): pData(NULL), DataSize(0), Allocated(0) { Set(pSrc, SrcSize); }
	CBuffer(int Size): pData(NULL), DataSize(0), Allocated(0) { Allocate(Size); }
	CBuffer(const CBuffer& Other): pData(NULL), DataSize(0), Allocated(0) { Set(Other.pData, Other.DataSize); }
	~CBuffer() { if (pData) n_free(pData); }

	void	Reserve(int Size);
	void	Clear();
	void	Trim(int Size) { n_assert(Size <= DataSize); DataSize = Size; }
	int		HashCode() const { Hash(pData, DataSize); }

	bool	IsValid() const { return !!pData; }
	void*	GetPtr() const { return pData; }
	int		GetSize() const { return DataSize; }
	int		GetCapacity() const { return Allocated; }
	void	Set(const void* pSrc, int SrcSize);
	void	Read(char* pDst, int StartIdx, int EndIdx);
	void	Write(const char* pSrc, int StartIdx, int EndIdx);

	void operator =(const CBuffer& Other) { Set(Other.pData, Other.DataSize); }
	bool operator ==(const CBuffer& Other) const { return !BinaryCompare(Other); }
	bool operator !=(const CBuffer& Other) const { return !!BinaryCompare(Other); }
	bool operator >(const CBuffer& Other) const { return BinaryCompare(Other) > 0; }
	bool operator <(const CBuffer& Other) const { return BinaryCompare(Other) < 0; }
	bool operator >=(const CBuffer& Other) const { return BinaryCompare(Other) >= 0; }
	bool operator <=(const CBuffer& Other) const { return BinaryCompare(Other) <= 0; }
};

inline void CBuffer::Allocate(int Size)
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

inline void CBuffer::Set(const void* pSrc, int SrcSize)
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

inline void CBuffer::Read(char* pDst, int StartIdx, int EndIdx)
{
	n_assert(pDst &&
		StartIdx >= 0 &&
		StartIdx < Allocated &&
		EndIdx >= 0 &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pDst, pData + StartIdx, EndIdx - StartIdx + 1);
}
//---------------------------------------------------------------------

inline void CBuffer::Write(const char* pSrc, int StartIdx, int EndIdx)
{
	n_assert(pSrc &&
		StartIdx >= 0 &&
		StartIdx < Allocated &&
		EndIdx >= 0 &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pData + StartIdx, pSrc, EndIdx - StartIdx + 1);
	if (DataSize < EndIdx) DataSize = EndIdx + 1;
}
//---------------------------------------------------------------------

inline void CBuffer::Reserve(int Size)
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