#include "RAMData.h"
#include <IO/Stream.h>

namespace Data
{

// NB: stream must be opened
CRAMDataMappedStream::CRAMDataMappedStream(IO::PStream Stream) : _Stream(Stream)
{
	_pData = (_Stream && _Stream->IsOpened()) ? _Stream->Map() : nullptr;
}
//---------------------------------------------------------------------

CRAMDataMappedStream::~CRAMDataMappedStream()
{
	if (_pData && _Stream) _Stream->Unmap();
}
//---------------------------------------------------------------------

}