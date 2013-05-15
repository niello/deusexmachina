#pragma once
#ifndef __DEM_L1_TEXT_READER_H__
#define __DEM_L1_TEXT_READER_H__

#include <IO/StreamReader.h>
#include <util/nstring.h>

// Text data reader, aware of line endings

class nString;

namespace IO
{

class CTextReader: public CStreamReader
{
public:

	CTextReader(CStream& SrcStream): CStreamReader(SrcStream) { }

	bool ReadLine(char* pOutValue, DWORD MaxLen = MAX_DWORD);
	bool ReadLine(nString& OutValue);
	bool ReadLines(nArray<nString>& OutValues, int Count = -1);
	bool ReadAll(nString& OutValue);
};

}

#endif
