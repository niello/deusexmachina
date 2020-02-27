#include "MemStream.h"
#include <Data/Buffer.h>
#include <memory.h>
#include <algorithm>

namespace IO
{

CMemStream::CMemStream(UPTR InitialBufferSize)
	: CMemStream(std::make_unique<Data::CBufferMalloc>(InitialBufferSize))
{
}
//---------------------------------------------------------------------

CMemStream::CMemStream(void* pData, UPTR BufferSize, UPTR DataSize)
	: _pData((char*)pData)
	, _BufferSize(BufferSize)
	, _UnusedStart(DataSize)
	, _ReadOnly(false)
	, _BufferOwned(false)
{
}
//---------------------------------------------------------------------

CMemStream::CMemStream(const void* pData, UPTR BufferSize, UPTR DataSize)
	: _pConstData((const char*)pData)
	, _BufferSize(BufferSize)
	, _UnusedStart(DataSize)
	, _ReadOnly(false)
	, _BufferOwned(false)
{
}
//---------------------------------------------------------------------

CMemStream::CMemStream(Data::IBuffer& Buffer)
	: _pBuffer(&Buffer)
	, _BufferOwned(false)
{
	if (_pBuffer)
	{
		if (_pData = (char*)_pBuffer->GetPtr())
			_ReadOnly = false;
		else if (_pConstData = (const char*)_pBuffer->GetConstPtr())
			_ReadOnly = true;

		_BufferSize = _pBuffer->GetSize();
	}
}
//---------------------------------------------------------------------

CMemStream::CMemStream(Data::PBuffer&& Buffer)
	: _pBuffer(Buffer.release())
	, _BufferOwned(true)
{
	if (_pBuffer)
	{
		if (_pData = (char*)_pBuffer->GetPtr())
			_ReadOnly = false;
		else if (_pConstData = (const char*)_pBuffer->GetConstPtr())
			_ReadOnly = true;

		_BufferSize = _pBuffer->GetSize();
	}
}
//---------------------------------------------------------------------

Data::PBuffer CMemStream::Detach()
{
	if (!_pBuffer || !_BufferOwned) return Data::PBuffer();

	_pData = nullptr;
	_BufferSize = 0;
	_Pos = 0;
	_UnusedStart = 0;
	auto OutBuffer = Data::PBuffer(_pBuffer);
	_pBuffer = nullptr;
	return OutBuffer;
}
//---------------------------------------------------------------------

void CMemStream::Close()
{
	_pData = nullptr;
	_BufferSize = 0;
	_Pos = 0;
	_UnusedStart = 0;
	if (_pBuffer && _BufferOwned) delete _pBuffer;
	_pBuffer = nullptr;
}
//---------------------------------------------------------------------

UPTR CMemStream::Read(void* pData, UPTR Size)
{
	if (!_pConstData) return 0;

	const UPTR BytesToRead = std::min(Size, _BufferSize - _Pos);
	if (BytesToRead > 0)
	{
		memcpy(pData, _pConstData + _Pos, BytesToRead);
		_Pos += BytesToRead;
	}
	return BytesToRead;
}
//---------------------------------------------------------------------

UPTR CMemStream::Write(const void* pData, UPTR Size)
{
	if (!_pData || _ReadOnly) return 0;

	const auto NewSize = _Pos + Size;
	if (NewSize > _BufferSize)
	{
		if (_pBuffer)
		{
			if (auto pNewData = _pBuffer->Resize(NewSize))
			{
				_pData = (char*)pNewData;
				_BufferSize = _pBuffer->GetSize();
			}
			else
				Size = _BufferSize - _Pos;
		}
		else
			Size = _BufferSize - _Pos;
	}

	if (Size)
	{
		memcpy(_pData + _Pos, pData, Size);
		_Pos += Size;
		if (_Pos > _UnusedStart) _UnusedStart = _Pos;
	}

	return Size;
}
//---------------------------------------------------------------------

UPTR CMemStream::Fill(U8 Value, UPTR Size)
{
	if (!_pData || _ReadOnly) return 0;

	const auto NewSize = _Pos + Size;
	if (NewSize > _BufferSize)
	{
		if (_pBuffer)
		{
			if (auto pNewData = _pBuffer->Resize(NewSize))
			{
				_pData = (char*)pNewData;
				_BufferSize = _pBuffer->GetSize();
			}
			else
				Size = _BufferSize - _Pos;
		}
		else
			Size = _BufferSize - _Pos;
	}

	if (Size)
	{
		memset(_pData + _Pos, Value, Size);
		_Pos += Size;
		if (_Pos > _UnusedStart) _UnusedStart = _Pos;
	}

	return Size;
}
//---------------------------------------------------------------------

bool CMemStream::Seek(I64 Offset, ESeekOrigin Origin)
{
	I64 SeekPos;
	switch (Origin)
	{
		case Seek_Begin:   SeekPos = Offset; break;
		case Seek_Current: SeekPos = _Pos + Offset; break;
		case Seek_End:     SeekPos = _BufferSize + Offset; break;
		default:           ::Sys::Error("CMemStream::Seek() > unknown ESeekOrigin");
	}
	_Pos = static_cast<UPTR>(std::clamp<I64>(SeekPos, 0, _BufferSize));
	return _Pos == SeekPos;
}
//---------------------------------------------------------------------

void* CMemStream::Map()
{
	// FIXME: MapRead / MapWrite? Must not return read-only data for writing!
	return _pData;
}
//---------------------------------------------------------------------

Data::PBuffer CMemStream::ReadAll()
{
	if (!IsOpened() || IsEOF()) return nullptr;

	const auto Size = _BufferSize - _Pos;
	auto Buffer = std::make_unique<Data::CBufferMalloc>(Size);
	if (Size) Read(Buffer->GetPtr(), Size);

	return Buffer;
}
//---------------------------------------------------------------------

}