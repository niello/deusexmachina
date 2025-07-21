#pragma once
#include <IO/Stream.h>

// Text data reader, aware of line endings

namespace IO
{

class CTextReader final
{
protected:

	IStream& Stream;

public:

	CTextReader(IStream& SrcStream) : Stream(SrcStream) {}

	bool ReadLine(char* pOutValue, UPTR MaxLen = (UPTR)(-1));

	IStream& GetStream() const { return Stream; }
};

}
