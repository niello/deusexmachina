#pragma once
#ifndef __DEM_L1_MEM_STREAM_H__
#define __DEM_L1_MEM_STREAM_H__

#include <IO/Stream.h>

// RAM access stream
// Partially based on Nebula 3 (c) IO::MemoryStream class

namespace IO
{

class CMemStream: public IStream
{
protected:

	union
	{
		char*		pBuffer;
		const char*	pConstBuffer;
	};

	UPTR	Pos;
	UPTR	DataSize;
	UPTR	AllocSize;
	bool	SelfAlloc;

	void			Allocate(UPTR AddedBytes);

public:

	CMemStream(): pBuffer(nullptr), Pos(0), DataSize(0), AllocSize(0), SelfAlloc(false) {}
	virtual ~CMemStream() override { if (IsOpened()) Close(); }

	bool			Open(void* pData, UPTR Size);
	bool			Open(const void* pData, UPTR Size);
	virtual bool	Open() override {}
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

inline bool CMemStream::Open(void* pData, UPTR Size)
{
	pBuffer = (char*)pData;
	DataSize = Size;
	return Open();
}
//---------------------------------------------------------------------

inline bool CMemStream::Open(const void* pData, UPTR Size)
{
	pConstBuffer = (const char*)pData;
	DataSize = Size;
	return Open();
}
//---------------------------------------------------------------------

}

#endif