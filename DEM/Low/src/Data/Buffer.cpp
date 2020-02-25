#include "Buffer.h"
#include <IO/Stream.h>

namespace Data
{

void* CBufferMalloc::Resize(UPTR NewSize)
{
	if (auto pNewData = n_realloc(_pData, NewSize))
	{
		_pData = pNewData;
		_Size = NewSize;
		return _pData;
	}
	return nullptr;
}
//---------------------------------------------------------------------

void* CBufferMallocAligned::Resize(UPTR NewSize)
{
	if (auto pNewData = n_realloc_aligned(_pData, NewSize, _Alignment))
	{
		_pData = pNewData;
		_Size = NewSize;
		return _pData;
	}
	return nullptr;
}
//---------------------------------------------------------------------

CBufferMappedStream::CBufferMappedStream(IO::PStream Stream)
	: _Stream(Stream)
{
	_pData = (_Stream && _Stream->IsOpened()) ? _Stream->Map() : nullptr;
}
//---------------------------------------------------------------------

CBufferMappedStream::~CBufferMappedStream()
{
	if (_pData && _Stream) _Stream->Unmap();
}
//---------------------------------------------------------------------

void* CBufferMappedStream::GetPtr()
{
	return (_Stream && _Stream->CanWrite()) ? _pData : nullptr;
}
//---------------------------------------------------------------------

const void* CBufferMappedStream::GetConstPtr() const
{
	return (_Stream && _Stream->CanRead()) ? _pData : nullptr;
}
//---------------------------------------------------------------------

UPTR CBufferMappedStream::GetSize() const
{
	return _Stream ? static_cast<UPTR>(_Stream->GetSize()) : 0;
}
//---------------------------------------------------------------------

}