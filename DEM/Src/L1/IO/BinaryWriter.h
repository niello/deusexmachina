#pragma once
#ifndef __DEM_L1_BINARY_WRITER_H__
#define __DEM_L1_BINARY_WRITER_H__

#include <IO/StreamWriter.h>
#include <Data/Params.h>
#include <util/nstring.h>

// Binary data serializer

class nString;

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

	bool				WriteString(LPCSTR Value);
	bool				WriteString(const nString& Value);
	bool				WriteParams(const Data::CParams& Value);
	bool				WriteParams(const Data::CParams& Value, const Data::CDataScheme& Scheme) { DWORD Dummy; return WriteParamsByScheme(Value, Scheme, Dummy); }
	bool				WriteParam(const Data::CParam& Value) { return Write(Value.GetName()) && Write(Value.GetRawValue()); }
	bool				WriteData(const Data::CData& Value);

	template<class T>
	bool				Write(const T& Value) { return Stream.Write(&Value, sizeof(T)) == sizeof(T); }
	template<> bool		Write<LPSTR>(const LPSTR& Value) { return WriteString(Value); }
	template<> bool		Write<LPCSTR>(const LPCSTR& Value) { return WriteString(Value); }
	template<> bool		Write<nString>(const nString& Value) { return WriteString(Value); }
	template<> bool		Write<CStrID>(const CStrID& Value) { return WriteString(Value.CStr()); }
	template<> bool		Write<Data::CParams>(const Data::CParams& Value) { return WriteParams(Value); }
	template<> bool		Write<Data::PParams>(const Data::PParams& Value) { return Value.IsValid() ? WriteParams(*Value) : true; }
	template<> bool		Write<Data::CParam>(const Data::CParam& Value) { return WriteParam(Value); }
	template<> bool		Write<Data::CData>(const Data::CData& Value) { return WriteData(Value); }
	template<> bool		Write<Data::CDataArray>(const Data::CDataArray& Value);
	template<> bool		Write<Data::PDataArray>(const Data::PDataArray& Value) { return Value.IsValid() ? Write<Data::CDataArray>(*Value) : true; }
	template<> bool		Write<Data::CBuffer>(const Data::CBuffer& Value);
};

inline bool CBinaryWriter::WriteString(LPCSTR Value)
{
	short Len = Value ? (short)strlen(Value) : 0;
	return Write(Len) && (!Len || Stream.Write(Value, Len) == Len);
}
//---------------------------------------------------------------------

inline bool CBinaryWriter::WriteString(const nString& Value)
{
	return Write<short>(Value.Length()) &&
		(!Value.Length() || Stream.Write(Value.CStr(), Value.Length()) == Value.Length());
}
//---------------------------------------------------------------------

inline bool CBinaryWriter::WriteParams(const Data::CParams& Value)
{
	if (!Write<short>(Value.GetCount())) FAIL;
	for (int i = 0; i < Value.GetCount(); i++)
		if (!WriteParam(Value.Get(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
