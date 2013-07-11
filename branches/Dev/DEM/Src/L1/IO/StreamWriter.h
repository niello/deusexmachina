#pragma once
#ifndef __DEM_L1_STREAM_WRITER_H__
#define __DEM_L1_STREAM_WRITER_H__

#include <IO/Stream.h>
#include <string.h> // strlen

// Generic stream writer. Derive classes to write structured/formatted data to streams.

namespace IO
{

class CStreamWriter
{
protected:

	CStream& Stream;

public:

	CStreamWriter(CStream& DestStream): Stream(DestStream) { n_assert(Stream.IsOpen()); }

	bool		WriteCharString(const char* pString);
	CStream&	GetStream() const { return Stream; }
};

inline bool CStreamWriter::WriteCharString(const char* pString)
{
	DWORD Len = pString ? strlen(pString) : 0;
	return Stream.Write(pString, Len) == Len;
}
//---------------------------------------------------------------------

}

#endif
