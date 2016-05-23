#pragma once
#ifndef __DEM_L1_SCOPED_STREAM_H__
#define __DEM_L1_SCOPED_STREAM_H__

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
	virtual ~CScopedStream() { if (IsOpen()) Close(); }

	bool			SetScope(U64 Offset, U64 Size);

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	Close();
	virtual UPTR	Read(void* pData, UPTR Size) { return HostStream->Read(pData, Size); }
	virtual UPTR	Write(const void* pData, UPTR Size) { return HostStream->Write(pData, Size); }
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin);
	virtual void	Flush() { HostStream->Flush(); }
	virtual void*	Map() { return ((char*)HostStream->Map()) + ScopeOffset; }
	virtual void	Unmap() { HostStream->Unmap(); }

	virtual U64		GetSize() const { return ScopeSize; }
	virtual U64		GetPosition() const { return HostStream->GetPosition() - ScopeOffset; }
	virtual bool	IsEOF() const;
	virtual bool	CanRead() const { return HostStream->CanRead(); }
	virtual bool	CanWrite() const { return HostStream->CanWrite(); }
	virtual bool	CanSeek() const { return HostStream->CanSeek(); }
	virtual bool	CanBeMapped() const { return HostStream->CanBeMapped(); }
};

typedef Ptr<CScopedStream> PScopedStream;

}

#endif