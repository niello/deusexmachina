#pragma once
#include <IO/Stream.h>

// RAM access stream

namespace IO
{

class CMemStream: public IStream
{
protected:

	union
	{
		char*		pBuffer = nullptr;
		const char*	pConstBuffer;
	};

	UPTR	Pos = 0;
	UPTR	DataSize = 0;
	UPTR	AllocSize = 0;
	bool	SelfAlloc = false;

	void			Allocate(UPTR AddedBytes);

public:

	CMemStream(void* pData, UPTR Size) : pBuffer((char*)pData), DataSize(Size) {}
	CMemStream(const void* pData, UPTR Size) : pConstBuffer((const char*)pData), DataSize(Size) {}
	virtual ~CMemStream() override { if (IsOpened()) Close(); }

	virtual void	Close() override;
	virtual UPTR	Read(void* pData, UPTR Size) override;
	virtual UPTR	Write(const void* pData, UPTR Size) override;
	UPTR			Fill(U8 Value, UPTR ByteCount);
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) override;
	virtual U64		Tell() const override { return Pos; }
	virtual bool    Truncate() override { DataSize = Pos; }
	virtual void	Flush() override {}
	virtual void*	Map() override;

	virtual U64		GetSize() const override { return DataSize; }
	virtual bool	IsOpened() const override { return !!pBuffer; }
	virtual bool    IsMapped() const override { return true; }
	virtual bool	IsEOF() const override;
	virtual bool	CanRead() const override { OK; }
	virtual bool	CanWrite() const override { OK; }
	virtual bool	CanSeek() const override { OK; }
	virtual bool	CanBeMapped() const override { OK; }
};

typedef Ptr<CMemStream> PMemStream;

}
