#pragma once
#ifndef __DEM_L1_BINARY_READER_H__
#define __DEM_L1_BINARY_READER_H__

#include <IO/StreamReader.h>
#include <Data/Params.h>
#include <Data/String.h>

// Binary data reader

class CString;

namespace Data
{
	class CDataScheme;
	typedef Ptr<class CDataArray> PDataArray;
}

namespace IO
{

class CBinaryReader: public CStreamReader
{
public:

	CBinaryReader(CStream& SrcStream): CStreamReader(SrcStream) { }

	bool				ReadString(char* OutValue, UPTR MaxLen);
	bool				ReadString(char*& OutValue); // Allocates memory
	bool				ReadString(CString& OutValue);
	bool				ReadParams(Data::CParams& OutValue);
	bool				ReadParam(Data::CParam& OutValue);
	bool				ReadData(Data::CData& OutValue);
	bool				ReadDataArray(Data::CDataArray& OutValue);

	template<class T>
	T					Read() { T Val; n_assert(Read<T>(Val)); return Val; }
	template<class T>
	bool				Read(T& OutValue) { return Stream.Read(&OutValue, sizeof(T)) == sizeof(T); }
	template<> bool		Read<char*>(char*& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<CString>(CString& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<CStrID>(CStrID& OutValue);
	template<> bool		Read<Data::CDataArray>(Data::CDataArray& OutValue) { return ReadDataArray(OutValue); }
	template<> bool		Read<Data::PDataArray>(Data::PDataArray& OutValue) { return OutValue.IsValidPtr() ? ReadDataArray(*OutValue) : true; }
	//template<> bool		Read<CParams>(const CParams& OutValue) { return WriteParams(OutValue); }
	//template<> bool		Read<PParams>(const PParams& OutValue) { return OutValue.IsValid() ? Read(*OutValue) : true; }
	//template<> bool		Read<CParam>(const CParam& OutValue) { return WriteParam(OutValue); }
	template<> bool		Read<Data::CData>(Data::CData& OutValue) { return ReadData(OutValue); }
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
