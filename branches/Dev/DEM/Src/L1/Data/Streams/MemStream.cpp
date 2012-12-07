#include "MemStream.h"

#include <Data/DataServer.h>

namespace Data
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

DWORD CMemStream::Read(void* pData, DWORD Size)
{
	n_assert(IsOpen() && pBuffer && !IsMapped() && (AccessMode & SAM_READ));
	DWORD BytesToRead = n_min(Size, DataSize - Pos);
	if (BytesToRead > 0)
	{
		memcpy(pData, pBuffer + Pos, BytesToRead);
		Pos += BytesToRead;
	}
	return BytesToRead;
}
//---------------------------------------------------------------------

DWORD CMemStream::Write(const void* pData, DWORD Size)
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

bool CMemStream::Seek(int Offset, ESeekOrigin Origin)
{
	int SeekPos;
	switch (Origin)
	{
		case SSO_BEGIN:		SeekPos = Offset; break;
		case SSO_CURRENT:	SeekPos = Pos + Offset; break;
		case SSO_END:		SeekPos = DataSize + Offset; break;
		default:			n_error("CMemStream::Seek -> Unknown origin");
	}
	Pos = n_iclamp(SeekPos, 0, DataSize);
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