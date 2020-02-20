#include "MemStream.h"
#include <memory.h>
#include <algorithm>

namespace IO
{

bool CMemStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpened());
	Pos = (Mode == SAM_APPEND) ? DataSize : 0;
	OK;
}
//---------------------------------------------------------------------

void CMemStream::Close()
{
	n_assert(IsOpened());
	if (IsMapped()) Unmap();
	if (SelfAlloc && pBuffer) n_free(pBuffer);
	pBuffer = nullptr;
}
//---------------------------------------------------------------------

UPTR CMemStream::Read(void* pData, UPTR Size)
{
	n_assert_dbg(IsOpened() && pConstBuffer && !IsMapped());
	UPTR BytesToRead = std::min(Size, DataSize - Pos);
	if (BytesToRead > 0)
	{
		memcpy(pData, pConstBuffer + Pos, BytesToRead);
		Pos += BytesToRead;
	}
	return BytesToRead;
}
//---------------------------------------------------------------------

void CMemStream::Allocate(UPTR AddedBytes)
{
	if (Pos + AddedBytes <= AllocSize) return;

	AllocSize = Pos + AddedBytes;
	if (SelfAlloc) pBuffer = (char*)n_realloc(pBuffer, AllocSize);
	else
	{
		void* pOld = pBuffer;
		pBuffer = (char*)n_malloc(AllocSize);
		SelfAlloc = true;
		if (pOld) memcpy(pBuffer, pOld, DataSize);
	}
	n_assert(pBuffer);
}
//---------------------------------------------------------------------

UPTR CMemStream::Write(const void* pData, UPTR Size)
{
	n_assert_dbg(IsOpened() && !IsMapped());
	Allocate(Size);
	memcpy(pBuffer + Pos, pData, Size);
	Pos += Size;
	if (Pos > DataSize) DataSize = Pos;
	return Size;
}
//---------------------------------------------------------------------

UPTR CMemStream::Fill(U8 Value, UPTR ByteCount)
{
	n_assert_dbg(IsOpened() && !IsMapped());
	Allocate(ByteCount);
	memset(pBuffer + Pos, Value, ByteCount);
	Pos += ByteCount;
	if (Pos > DataSize) DataSize = Pos;
	return ByteCount;
}
//---------------------------------------------------------------------

bool CMemStream::Seek(I64 Offset, ESeekOrigin Origin)
{
	I64 SeekPos;
	switch (Origin)
	{
		case Seek_Begin:	SeekPos = Offset; break;
		case Seek_Current:	SeekPos = Pos + Offset; break;
		case Seek_End:		SeekPos = DataSize + Offset; break;
		default:			Sys::Error("CMemStream::Seek -> Unknown origin");
	}
	Pos = static_cast<UPTR>(std::clamp<I64>(SeekPos, 0, DataSize));
	return Pos == SeekPos;
}
//---------------------------------------------------------------------

bool CMemStream::IsEOF() const
{
	n_assert(IsOpened() && !IsMapped() && Pos >= 0 && Pos <= DataSize);
	return Pos == DataSize;
}
//---------------------------------------------------------------------

void* CMemStream::Map()
{
	//Stream::Map();
	return pBuffer;
}
//---------------------------------------------------------------------

}