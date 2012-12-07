#pragma once
#ifndef __DEM_L1_BINARY_READER_H__
#define __DEM_L1_BINARY_READER_H__

#include <Data/StreamReader.h>
#include <Data/Params.h>
#include <util/nstring.h>

// Binary data reader

class nString;

namespace Data
{
class CDataScheme;

class CBinaryReader: public CStreamReader
{
public:

	CBinaryReader(CStream& SrcStream): CStreamReader(SrcStream) { }

	bool				ReadString(char* OutValue, DWORD MaxLen);
	bool				ReadString(char*& OutValue); // Allocates memory
	bool				ReadString(nString& OutValue);
	bool				ReadParams(CParams& OutValue);
	bool				ReadParam(CParam& OutValue);
	bool				ReadData(CData& OutValue);

	template<class T>
	T					Read() { T Val; n_assert(Read<T>(Val)); return Val; }
	template<class T>
	bool				Read(T& OutValue) { return Stream.Read(&OutValue, sizeof(T)) == sizeof(T); }
	template<> bool		Read<char*>(char*& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<nString>(nString& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<CStrID>(CStrID& OutValue);
	//template<> bool		Read<CParams>(const CParams& OutValue) { return WriteParams(OutValue); }
	//template<> bool		Read<PParams>(const PParams& OutValue) { return OutValue.isvalid() ? WriteParams(*OutValue) : true; }
	//template<> bool		Read<CParam>(const CParam& OutValue) { return WriteParam(OutValue); }
	template<> bool		Read<CData>(CData& OutValue) { return ReadData(OutValue); }
};

template<> inline bool CBinaryReader::Read<CStrID>(CStrID& OutValue)
{
	char Buffer[512]; // Some sane size. Can use ReadString(char*&) to allocate dynamically.
	if (!ReadString(Buffer, sizeof(Buffer))) FAIL;
	OutValue = CStrID(Buffer);
	OK;
}
//---------------------------------------------------------------------

}

#endif
