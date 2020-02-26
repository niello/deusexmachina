#pragma once
#include <IO/Stream.h>

// Stream that works with a memory region. It can be RAM, mapped file or any other source
// represented as a memory address. Sources are divided by following attributes:
// - constant (read-only, C) or mutable (M)
// - fixed-size (F) or resizeable (R)
// - owned (O) or not owned (N)
// See constructor annotations to choose what you need.

namespace IO
{

class CMemStream: public IStream
{
protected:

	union
	{
		char*		_pData = nullptr;
		const char*	_pConstData;
	};

	UPTR           _BufferSize = 0;

	Data::IBuffer* _pBuffer = nullptr;

	UPTR           _Pos = 0;
	UPTR           _UnusedStart = 0; // To track part not written into
	bool           _ReadOnly : 1;
	bool           _BufferOwned : 1;

public:

	CMemStream(); // MRO
	CMemStream(void* pData, UPTR BufferSize, UPTR DataSize = 0); // CFN
	CMemStream(const void* pData, UPTR BufferSize, UPTR DataSize = 0); // MFN
	CMemStream(Data::IBuffer& Buffer); // CFN/MFN/MRN, depends on the buffer implementation
	CMemStream(Data::PBuffer&& Buffer); // CFO/MFO/MRO, depends on the buffer implementation
	virtual ~CMemStream() override { if (IsOpened()) Close(); }

	Data::PBuffer   Detach();

	virtual void	Close() override;
	virtual UPTR	Read(void* pData, UPTR Size) override;
	virtual UPTR	Write(const void* pData, UPTR Size) override;
	UPTR			Fill(U8 Value, UPTR Size);
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) override;
	virtual U64		Tell() const override { return _Pos; }
	virtual bool    Truncate() override { if (_UnusedStart > _Pos) _UnusedStart = _Pos; OK; }
	virtual void	Flush() override {}
	virtual void*	Map() override;
	virtual void	Unmap() override {}

	virtual U64		GetSize() const override { return _BufferSize; }
	virtual bool	IsOpened() const override { return _pData || _pBuffer; }
	virtual bool    IsMapped() const override { return false; }
	virtual bool	IsEOF() const override { return _pConstData ? (_Pos >= _BufferSize) : true; }
	virtual bool	CanRead() const override { OK; }
	virtual bool	CanWrite() const override { OK; }
	virtual bool	CanSeek() const override { OK; }
	virtual bool	CanBeMapped() const override { OK; }

	virtual Data::PBuffer ReadAll() override;
};

typedef Ptr<CMemStream> PMemStream;

}
