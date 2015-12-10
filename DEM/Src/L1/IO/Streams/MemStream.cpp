#include "MemStream.h"

#include <Data/DataServer.h>

namespace IO
{

bool CMemStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpen());
	if (!CStream::Open(Mode, Pattern)) FAIL;
	Pos = (Mode == SAM_APPEND) ? DataSize : 0;
	Flags.Set(IS_OPEN);
	OK;
}
//---------------------------------------------------------------------

void CMemStream::Close()
{
	n_assert(IsOpen());
	if (IsMapped()) Unmap();
	if (SelfAlloc && pBuffer) n_free(pBuffer);
	pBuffer = NULL;
	Flags.Clear(IS_OPEN);
	//CStream::Close();
}
//---------------------------------------------------------------------

UPTR CMemStream::Read(void* pData, UPTR Size)
{
	n_assert(IsOpen() && pConstBuffer && !IsMapped() && (AccessMode & SAM_READ));
	UPTR BytesToRead = n_min(Size, DataSize - Pos);
	if (BytesToRead > 0)
	{
		memcpy(pData, pConstBuffer + Pos, BytesToRead);
		Pos += BytesToRead;
	}
	return BytesToRead;
}
//---------------------------------------------------------------------

UPTR CMemStream::Write(const void* pData, UPTR Size)
{
	n_assert(IsOpen() && !IsMapped() && ((AccessMode & SAM_WRITE) || (AccessMode & SAM_APPEND)));

	if (Pos + Size > AllocSize)
	{
		AllocSize = Pos + Size;
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

	memcpy(pBuffer + Pos, pData, Size);
	Pos += Size;
	if (Pos > DataSize) DataSize = Pos;
	return Size;
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
	Pos = (IPTR)Clamp<I64>(SeekPos, 0, DataSize);
	return Pos == SeekPos;
}
//---------------------------------------------------------------------

bool CMemStream::IsEOF() const
{
	n_assert(IsOpen() && !IsMapped() && Pos >= 0 && Pos <= DataSize);
	return Pos == DataSize;
}
//---------------------------------------------------------------------

void* CMemStream::Map()
{
    //n_assert(this->IsOpen());
    //Stream::Map();
    //n_assert(this->GetSize() > 0);
    //return this->buffer;
	return NULL;
}
//---------------------------------------------------------------------

}