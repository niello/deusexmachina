#pragma once
#include <IO/Stream.h>
#include <Data/String.h>
#include <Data/Array.h>

// Text data reader, aware of line endings

class CString;

namespace IO
{

class CTextReader final
{
protected:

	IStream& Stream;

public:

	CTextReader(IStream& SrcStream) : Stream(SrcStream) {}

	bool ReadLine(char* pOutValue, UPTR MaxLen = (UPTR)(-1));
	bool ReadLine(CString& OutValue);
	bool ReadLines(CArray<CString>& OutValues, UPTR Count = (UPTR)(-1));
	bool ReadAll(CString& OutValue);

	IStream& GetStream() const { return Stream; }
};

}
