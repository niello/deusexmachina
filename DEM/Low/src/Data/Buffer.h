#pragma once
#include <StdDEM.h>
#include <Data/Type.h>
#include <Data/Hash.h>
#include <System/System.h>
#include <memory>

// The CBuffer class encapsulates a chunk of raw memory into a C++ object which can be copied, compared and hashed.
// Based on mangalore Util::Blob_(C) 2006 Radon Labs GmbH

namespace Data
{
typedef std::unique_ptr<class CBuffer> PBuffer;

class CBuffer
{
protected:

	char*	pData = nullptr;
	UPTR	DataSize = 0;
	UPTR	Allocated = 0;

	void	Allocate(UPTR Size);
	int		BinaryCompare(const CBuffer& Other) const;

public:

	CBuffer() {}
	CBuffer(const void* pSrc, UPTR SrcSize) { Set(pSrc, SrcSize); }
	CBuffer(UPTR Size) { Allocate(Size); }
	CBuffer(const CBuffer& Other) { Set(Other.pData, Other.DataSize); }
	CBuffer(CBuffer&& Other): pData(Other.pData), DataSize(Other.DataSize), Allocated(Other.Allocated) { Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
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
	void operator =(CBuffer&& Other) { pData = Other.pData; DataSize = Other.DataSize; Allocated = Other.Allocated; Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
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
		pData = nullptr;
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

//???PBuffer?
DECLARE_TYPE(Data::CBuffer, 9)
#define TBuffer DATA_TYPE(Data::CBuffer)
