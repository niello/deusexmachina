#pragma once
#ifndef __DEM_L1_STREAM_READER_H__
#define __DEM_L1_STREAM_READER_H__

#include <IO/Stream.h>

// Generic stream reader. Derive classes to read structured/formatted data from streams.

namespace IO
{

class CStreamReader
{
protected:

	CStream& Stream;

public:

	CStreamReader(CStream& SrcStream): Stream(SrcStream) { n_assert(Stream.IsOpen()); }

	CStream& GetStream() const { return Stream; }
};

}

#endif
