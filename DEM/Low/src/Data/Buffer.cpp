#include "Buffer.h"
#include <IO/Stream.h>

namespace Data
{

// NB: stream must be opened
CBufferMappedStream::CBufferMappedStream(IO::PStream Stream) : _Stream(Stream)
{
	_pData = (_Stream && _Stream->IsOpened()) ? _Stream->Map() : nullptr;
}
//---------------------------------------------------------------------

CBufferMappedStream::~CBufferMappedStream()
{
	if (_pData && _Stream) _Stream->Unmap();
}
//---------------------------------------------------------------------

UPTR CBufferMappedStream::GetSize() const
{
	return _Stream ? static_cast<UPTR>(_Stream->GetSize()) : 0;
}
//---------------------------------------------------------------------

}