#pragma once
#include <IO/IOFwd.h>
#include <Data/RefCounted.h>

// Base stream interface for byte sequence access.

namespace IO
{

class IStream: public Data::CRefCounted
{
public:

	virtual ~IStream() override = default;

	virtual bool	Open() = 0;
	virtual void	Close() = 0;
	virtual UPTR	Read(void* pData, UPTR Size) = 0;
	virtual UPTR	Write(const void* pData, UPTR Size) = 0;
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) = 0;
	virtual U64		Tell() const = 0;
	virtual bool    Truncate() = 0;
	virtual void	Flush() = 0;
	virtual void*	Map() = 0;
	virtual void	Unmap() = 0;

	virtual U64		GetSize() const = 0;
	virtual bool	IsOpened() const = 0;
	virtual bool    IsMapped() const = 0;
	virtual bool	IsEOF() const = 0;
	virtual bool	CanRead() const { FAIL; }
	virtual bool	CanWrite() const { FAIL; }
	virtual bool	CanSeek() const { FAIL; }
	virtual bool	CanBeMapped() const { FAIL; }
};

typedef Ptr<IStream> PStream;

}
