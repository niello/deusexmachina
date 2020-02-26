#pragma once
#include <Data/Type.h>
#include <Data/Hash.h>
#include <System/System.h>

// The CDataBuffer class encapsulates a chunk of raw memory into a C++ object which can be copied, compared and hashed

namespace Data
{

class CDataBuffer final
{
protected:

	char*	pData = nullptr;
	UPTR	DataSize = 0;
	UPTR	Allocated = 0;

	void	Allocate(UPTR Size);
	int		BinaryCompare(const CDataBuffer& Other) const;

public:

	CDataBuffer() = default;
	CDataBuffer(const void* pSrc, UPTR SrcSize) { Set(pSrc, SrcSize); }
	CDataBuffer(UPTR Size) { Allocate(Size); }
	CDataBuffer(const CDataBuffer& Other) { Set(Other.pData, Other.DataSize); }
	CDataBuffer(CDataBuffer&& Other): pData(Other.pData), DataSize(Other.DataSize), Allocated(Other.Allocated) { Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
	~CDataBuffer() { if (pData) n_free(pData); }

	void	Reserve(UPTR Size);
	void	Clear();
	void	Truncate(UPTR Size) { if (Size < DataSize) DataSize = Size; }
	int		HashCode() const { Hash(pData, DataSize); }

	bool	IsValid() const { return !!pData; }
	bool	IsEmpty() const { return !pData || !DataSize; }
	void*	GetPtr() const { return pData; }
	int		GetSize() const { return DataSize; }
	int		GetCapacity() const { return Allocated; }
	void	Set(const void* pSrc, UPTR SrcSize);
	void	Read(char* pDst, UPTR StartIdx, UPTR EndIdx);
	void	Write(const char* pSrc, UPTR StartIdx, UPTR EndIdx);

	void operator =(const CDataBuffer& Other) { Set(Other.pData, Other.DataSize); }
	void operator =(CDataBuffer&& Other) { pData = Other.pData; DataSize = Other.DataSize; Allocated = Other.Allocated; Other.pData = nullptr; Other.DataSize = 0; Other.Allocated = 0; }
	bool operator ==(const CDataBuffer& Other) const { return !BinaryCompare(Other); }
	bool operator !=(const CDataBuffer& Other) const { return !!BinaryCompare(Other); }
	bool operator >(const CDataBuffer& Other) const { return BinaryCompare(Other) > 0; }
	bool operator <(const CDataBuffer& Other) const { return BinaryCompare(Other) < 0; }
	bool operator >=(const CDataBuffer& Other) const { return BinaryCompare(Other) >= 0; }
	bool operator <=(const CDataBuffer& Other) const { return BinaryCompare(Other) <= 0; }
};

inline void CDataBuffer::Read(char* pDst, UPTR StartIdx, UPTR EndIdx)
{
	n_assert_dbg(pDst &&
		StartIdx < Allocated &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pDst, pData + StartIdx, EndIdx - StartIdx + 1);
}
//---------------------------------------------------------------------

inline void CDataBuffer::Write(const char* pSrc, UPTR StartIdx, UPTR EndIdx)
{
	n_assert_dbg(pSrc &&
		StartIdx < Allocated &&
		EndIdx < Allocated &&
		EndIdx >= StartIdx);

	memcpy(pData + StartIdx, pSrc, EndIdx - StartIdx + 1);
	if (DataSize < EndIdx) DataSize = EndIdx + 1;
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(Data::CDataBuffer, 9)
#define TBuffer DATA_TYPE(Data::CDataBuffer)
