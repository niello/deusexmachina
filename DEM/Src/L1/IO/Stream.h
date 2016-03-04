#pragma once
#ifndef __DEM_L1_STREAM_H__
#define __DEM_L1_STREAM_H__

#include <IO/IOFwd.h>
#include <Data/Flags.h>
#include <Data/RefCounted.h>

// Base stream interface for byte sequence access.
// Partially based on Nebula 3 (c) IO::Stream class

namespace IO
{

class CStream: public Data::CRefCounted
{
protected:

	enum
	{
		IS_OPEN		= 0x01,
		IS_MAPPED	= 0x02
	};

	EStreamAccessMode		AccessMode;
	EStreamAccessPattern	AccessPattern;
	Data::CFlags			Flags;

public:

	//CStream();
	virtual ~CStream() { n_assert(!IsOpen()); }

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) = 0;
	virtual void	Close() = 0;
	virtual UPTR	Read(void* pData, UPTR Size) = 0;
	virtual UPTR	Write(const void* pData, UPTR Size) = 0;
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) = 0;
	virtual void	Flush() = 0;
	virtual void*	Map() { n_assert(!IsMapped()); return NULL; }
	virtual void	Unmap() {}

	bool			IsOpen() const { return Flags.Is(IS_OPEN); }
	bool			IsMapped() const { return Flags.Is(IS_MAPPED); }
	virtual U64		GetSize() const = 0;
	virtual U64		GetPosition() const = 0;
	virtual bool	IsEOF() const = 0;
	virtual bool	CanRead() const { FAIL; }
	virtual bool	CanWrite() const { FAIL; }
	virtual bool	CanSeek() const { FAIL; }
	virtual bool	CanBeMapped() const { FAIL; }

	// For simple types (integrated or struct). Use stream writers for things like CString etc.
	template<class T>
	bool			Put(const T& Value) { return Write(&Value, sizeof(T)) == sizeof(T); }
	template<class T>
	bool			Get(T& Value) { return Read(&Value, sizeof(T)) == sizeof(T); }
	template<class T>
	T				Get() { T Value; Read(&Value, sizeof(T)); return Value; }
};

typedef Ptr<CStream> PStream;

}

#endif