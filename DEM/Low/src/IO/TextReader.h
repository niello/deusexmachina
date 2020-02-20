#pragma once
#ifndef __DEM_L1_TEXT_READER_H__
#define __DEM_L1_TEXT_READER_H__

#include <IO/StreamReader.h>
#include <Data/String.h>
#include <Data/Array.h>

// Text data reader, aware of line endings

class CString;

namespace IO
{

class CTextReader: public CStreamReader
{
public:

	CTextReader(IStream& SrcStream): CStreamReader(SrcStream) { }

	bool ReadLine(char* pOutValue, UPTR MaxLen = (UPTR)(-1));
	bool ReadLine(CString& OutValue);
	bool ReadLines(CArray<CString>& OutValues, UPTR Count = (UPTR)(-1));
	bool ReadAll(CString& OutValue);
};

}

#endif
