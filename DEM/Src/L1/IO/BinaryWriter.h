#pragma once
#ifndef __DEM_L1_BINARY_WRITER_H__
#define __DEM_L1_BINARY_WRITER_H__

#include <IO/StreamWriter.h>
#include <Data/Params.h>
#include <Data/String.h>

// Binary data serializer

class CString;

namespace Data
{
	class CDataScheme;
	class CBuffer;
	typedef Ptr<class CDataArray> PDataArray;
}

namespace IO
{

class CBinaryWriter: public CStreamWriter
{
protected:

	bool WriteParamsByScheme(const Data::CParams& Value, const Data::CDataScheme& Scheme, DWORD& Written);
	bool WriteDataAsOfType(const Data::CData& Value, int TypeID, Data::CFlags Flags);

public:

	CBinaryWriter(CStream& DestStream): CStreamWriter(DestStream) { }

	bool				WriteString(const char* Value);
	bool				WriteString(const CString& Value);
	bool				WriteParams(const Data::CParams& Value);
	bool				WriteParams(const Data::CParams& Value, const Data::CDataScheme& Scheme) { DWORD Dummy; return WriteParamsByScheme(Value, Scheme, Dummy); }
	bool				WriteParam(const Data::CParam& Value) { return Write(Value.GetName()) && Write(Value.GetRawValue()); }
	bool				WriteData(const Data::CData& Value);

	template<class T>
	bool				Write(const T& Value) { return Stream.Write(&Value, sizeof(T)) == sizeof(T); }
	template<> bool		Write<char*>(char* const& Value) { return WriteString(Value); }
	template<> bool		Write<const char*>(const char* const& Value) { return WriteString(Value); }
	template<> bool		Write<CString>(const CString& Value) { return WriteString(Value); }
	template<> bool		Write<CStrID>(const CStrID& Value) { return WriteString(Value.CStr()); }
	template<> bool		Write<Data::CParams>(const Data::CParams& Value) { return WriteParams(Value); }
	template<> bool		Write<Data::PParams>(const Data::PParams& Value) { return Value.IsValidPtr() ? WriteParams(*Value) : true; }
	template<> bool		Write<Data::CParam>(const Data::CParam& Value) { return WriteParam(Value); }
	template<> bool		Write<Data::CData>(const Data::CData& Value) { return WriteData(Value); }
	template<> bool		Write<Data::CDataArray>(const Data::CDataArray& Value);
	template<> bool		Write<Data::PDataArray>(const Data::PDataArray& Value) { return Value.IsValidPtr() ? Write<Data::CDataArray>(*Value) : true; }
	template<> bool		Write<Data::CBuffer>(const Data::CBuffer& Value);
};

inline bool CBinaryWriter::WriteString(const char* Value)
{
	short Len = Value ? (short)strlen(Value) : 0;
	return Write(Len) && (!Len || Stream.Write(Value, Len) == Len);
}
//---------------------------------------------------------------------

inline bool CBinaryWriter::WriteString(const CString& Value)
{
	return Write<U16>((U16)Value.GetLength()) &&
		(!Value.GetLength() || Stream.Write(Value.CStr(), Value.GetLength()) == Value.GetLength());
}
//---------------------------------------------------------------------

inline bool CBinaryWriter::WriteParams(const Data::CParams& Value)
{
	if (!Write<U16>(Value.GetCount())) FAIL;
	for (UPTR i = 0; i < Value.GetCount(); i++)
		if (!WriteParam(Value.Get(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
