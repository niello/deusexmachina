#include "DataBuffer.h"
#include <string.h>

namespace Data
{
DEFINE_TYPE(CDataBuffer, CDataBuffer())

void CDataBuffer::Allocate(UPTR Size)
{
	n_assert(!IsValid());
	pData = (char*)n_malloc(Size);
	Allocated = Size;
	DataSize = Size;
}
//---------------------------------------------------------------------

void CDataBuffer::Clear()
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

void CDataBuffer::Set(const void* pSrc, UPTR SrcSize)
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

void CDataBuffer::Reserve(UPTR Size)
{
	if (Allocated < Size)
	{
		Clear();
		Allocate(Size);
	}
	DataSize = Size;
}
//---------------------------------------------------------------------

int CDataBuffer::BinaryCompare(const CDataBuffer& Other) const
{
	n_assert(pData && Other.pData);
	if (DataSize == Other.DataSize) return memcmp(this->pData, Other.pData, this->DataSize);
	else if (DataSize > Other.DataSize) return 1;
	else return -1;
}
//---------------------------------------------------------------------

}