#pragma once
#include <Type.h>
#include <memory>
#include <algorithm>

// The CBuffer class encapsulates a chunk of raw memory into a C++ object which can be copied, compared and hashed.
// Based on mangalore Util::Blob_(C) 2006 Radon Labs GmbH

namespace Data
{
typedef std::unique_ptr<class CBuffer> PBuffer;

class CBuffer
{
protected:

	char*	pData = nullptr;
	size_t	DataSize = 0;
	size_t	Allocated = 0;

	void	Allocate(size_t Size);
	int		BinaryCompare(const CBuffer& Other) const;

public:

	CBuffer() {}
	CBuffer(const void* pSrc, size_t SrcSize) { Set(pSrc, SrcSize); }
	CBuffer(size_t Size) { Allocate(Size); }
	CBuffer(const CBuffer& Other) { Set(Other.pData, Other.DataSize); }
	CBuffer(CBuffer&& Other): pData(Other.pData), DataSize(Other.DataSize), Allocated(Other.Allocated) { Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
	~CBuffer() { if (pData) free(pData); }

	void	Reserve(size_t Size);
	void	Clear();
	void	Trim(size_t Size) { DataSize = std::min(DataSize, Size); }

	bool	IsValid() const { return !!pData; }
	bool	IsEmpty() const { return !pData || !DataSize; }
	void*	GetPtr() const { return pData; }
	int		GetSize() const { return DataSize; }
	int		GetCapacity() const { return Allocated; }
	void	Set(const void* pSrc, size_t SrcSize);
	void	Read(char* pDst, size_t StartIdx, size_t EndIdx);
	void	Write(const char* pSrc, size_t StartIdx, size_t EndIdx);

	void operator =(const CBuffer& Other) { Set(Other.pData, Other.DataSize); }
	void operator =(CBuffer&& Other) { pData = Other.pData; DataSize = Other.DataSize; Allocated = Other.Allocated; Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
	bool operator ==(const CBuffer& Other) const { return !BinaryCompare(Other); }
	bool operator !=(const CBuffer& Other) const { return !!BinaryCompare(Other); }
	bool operator >(const CBuffer& Other) const { return BinaryCompare(Other) > 0; }
	bool operator <(const CBuffer& Other) const { return BinaryCompare(Other) < 0; }
	bool operator >=(const CBuffer& Other) const { return BinaryCompare(Other) >= 0; }
	bool operator <=(const CBuffer& Other) const { return BinaryCompare(Other) <= 0; }
};

inline void CBuffer::Allocate(size_t Size)
{
	if (Size != DataSize)
	{
		if (pData) free(pData);
		pData = Size ? (char*)malloc(Size) : nullptr;
	}
	Allocated = Size;
	DataSize = Size;
}
//---------------------------------------------------------------------

inline void CBuffer::Clear()
{
	if (IsValid())
	{
		free(pData);
		pData = nullptr;
		DataSize = 0;
		Allocated = 0;
	}
}
//---------------------------------------------------------------------

inline void CBuffer::Set(const void* pSrc, size_t SrcSize)
{
	//n_assert(pSrc || !SrcSize);

	if (!pData || Allocated < SrcSize)
	{
		free(pData);
		pData = (char*)malloc(SrcSize);
		Allocated = SrcSize;
	}
	DataSize = SrcSize;
	if (SrcSize) memcpy(pData, pSrc, SrcSize);
}
//---------------------------------------------------------------------

inline void CBuffer::Read(char* pDst, size_t StartIdx, size_t EndIdx)
{
	//n_assert(pDst &&
	//	StartIdx < Allocated &&
	//	EndIdx < Allocated &&
	//	EndIdx >= StartIdx);

	memcpy(pDst, pData + StartIdx, EndIdx - StartIdx + 1);
}
//---------------------------------------------------------------------

inline void CBuffer::Write(const char* pSrc, size_t StartIdx, size_t EndIdx)
{
	//n_assert(pSrc &&
	//	StartIdx < Allocated &&
	//	EndIdx < Allocated &&
	//	EndIdx >= StartIdx);

	memcpy(pData + StartIdx, pSrc, EndIdx - StartIdx + 1);
	if (DataSize < EndIdx) DataSize = EndIdx + 1;
}
//---------------------------------------------------------------------

inline void CBuffer::Reserve(size_t Size)
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
