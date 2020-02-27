#include "Buffer.h"
#include <IO/Stream.h>

namespace Data
{
DEFINE_TYPE(CBufferMalloc, CBufferMalloc(0))

int IBuffer::Compare(const void* pOtherData, UPTR OtherSize) const
{
	auto MySize = GetSize();
	if (MySize != OtherSize) return MySize - OtherSize;

	auto pMyData = GetConstPtr();
	if (pMyData == pOtherData) return 0;

	return memcmp(pMyData, pOtherData, MySize);
}
//---------------------------------------------------------------------

CBufferMalloc::CBufferMalloc(const CBufferMalloc& Other)
	: _pData(Other._Size ? n_malloc(Other._Size) : nullptr)
	, _Size(Other._Size)
{
	if (_Size) memcpy(_pData, Other._pData, _Size);
}
//---------------------------------------------------------------------

CBufferMalloc& CBufferMalloc::operator =(const CBufferMalloc& Other)
{
	if (_pData) n_free(_pData);
	_Size = Other._Size;
	_pData = _Size ? n_malloc(_Size) : nullptr;
	if (_Size) memcpy(_pData, Other._pData, _Size);
	return *this;
}
//---------------------------------------------------------------------

CBufferMalloc& CBufferMalloc::operator =(CBufferMalloc&& Other)
{
	if (_pData) n_free(_pData);
	_pData = Other._pData;
	_Size = Other._Size;
	Other._pData = nullptr;
	Other._Size = 0;
	return *this;
}
//---------------------------------------------------------------------

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