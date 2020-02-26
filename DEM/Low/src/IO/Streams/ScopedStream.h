#pragma once
#include <IO/Stream.h>

// Stream filter that accesses only a part of host stream transparently for user code

namespace IO
{

class CScopedStream: public IStream
{
protected:

	PStream	HostStream;
	U64		ScopeOffset = 0;
	U64		ScopeSize = 0;

public:

	CScopedStream(PStream Host, U64 Offset, U64 Size);
	virtual ~CScopedStream() override { if (IsOpened()) Close(); }

	bool			SetScope(U64 Offset, U64 Size);

	virtual void	Close() override;
	virtual UPTR	Read(void* pData, UPTR Size) override { return HostStream->Read(pData, Size); }
	virtual UPTR	Write(const void* pData, UPTR Size) override { return HostStream->Write(pData, Size); }
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) override;
	virtual U64		Tell() const override { return HostStream->Tell() - ScopeOffset; }
	virtual bool    Truncate() override;
	virtual void	Flush() override { HostStream->Flush(); }
	virtual void*	Map() override { return ((char*)HostStream->Map()) + ScopeOffset; }
	virtual void	Unmap() override { HostStream->Unmap(); }

	virtual U64		GetSize() const override { return ScopeSize; }
	virtual bool	IsOpened() const override { return HostStream && HostStream->IsOpened() && ScopeSize; }
	virtual bool    IsMapped() const override { return HostStream && HostStream->IsMapped(); }
	virtual bool	IsEOF() const override;
	virtual bool	CanRead() const override { return HostStream && HostStream->CanRead(); }
	virtual bool	CanWrite() const override { return HostStream && HostStream->CanWrite(); }
	virtual bool	CanSeek() const override { return HostStream && HostStream->CanSeek(); }
	virtual bool	CanBeMapped() const override { return HostStream && HostStream->CanBeMapped(); }

	virtual Data::PBuffer ReadAll() override;
};

typedef Ptr<CScopedStream> PScopedStream;

}
