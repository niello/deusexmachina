#pragma once
#include <IO/Stream.h>

// Stream filter that accesses only a part of host stream transparently for user code

namespace IO
{

class CScopedStream: public CStream
{
protected:

	PStream	HostStream;
	U64		ScopeOffset;
	U64		ScopeSize;

public:

	CScopedStream(PStream Host): HostStream(Host) { Flags.SetTo(IS_OPEN, Host.IsValidPtr() && Host->IsOpen()); }
	virtual ~CScopedStream() override { if (IsOpen()) Close(); }

	bool			SetScope(U64 Offset, U64 Size);

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) override;
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
	virtual bool	IsEOF() const override;
	virtual bool	CanRead() const override { return HostStream->CanRead(); }
	virtual bool	CanWrite() const override { return HostStream->CanWrite(); }
	virtual bool	CanSeek() const override { return HostStream->CanSeek(); }
	virtual bool	CanBeMapped() const override { return HostStream->CanBeMapped(); }
};

typedef Ptr<CScopedStream> PScopedStream;

}
