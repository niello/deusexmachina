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

	IStream& Stream;

public:

	CStreamReader(IStream& SrcStream): Stream(SrcStream) { n_assert(Stream.IsOpened()); }

	IStream& GetStream() const { return Stream; }
};

}

#endif
