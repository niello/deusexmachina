#pragma once
#ifndef __DEM_L1_BINARY_WRITER_H__
#define __DEM_L1_BINARY_WRITER_H__

#include <Data/StreamWriter.h>
#include <Data/Params.h>
#include <util/nstring.h>

// Binary data serializer

class nString;

namespace Data
{
class CDataScheme;
class CBuffer;
typedef Ptr<class CDataArray> PDataArray;

class CBinaryWriter: public CStreamWriter
{
protected:

	bool WriteParamsByScheme(const CParams& Value, const CDataScheme& Scheme, DWORD& Written);
	bool WriteDataAsOfType(const CData& Value, int TypeID, CFlags Flags);

public:

	CBinaryWriter(CStream& DestStream): CStreamWriter(DestStream) { }

	bool				WriteString(LPCSTR Value);
	bool				WriteString(const nString& Value);
	bool				WriteParams(const CParams& Value);
	bool				WriteParams(const CParams& Value, const CDataScheme& Scheme) { DWORD Dummy; return WriteParamsByScheme(Value, Scheme, Dummy); }
	bool				WriteParam(const CParam& Value) { return Write(Value.GetName()) && Write(Value.GetRawValue()); }
	bool				WriteData(const CData& Value);

	template<class T>
	bool				Write(const T& Value) { return Stream.Write(&Value, sizeof(T)) == sizeof(T); }
	template<> bool		Write<LPSTR>(const LPSTR& Value) { return WriteString(Value); }
	template<> bool		Write<LPCSTR>(const LPCSTR& Value) { return WriteString(Value); }
	template<> bool		Write<nString>(const nString& Value) { return WriteString(Value); }
	template<> bool		Write<CStrID>(const CStrID& Value) { return WriteString(Value.CStr()); }
	template<> bool		Write<CParams>(const CParams& Value) { return WriteParams(Value); }
	template<> bool		Write<PParams>(const PParams& Value) { return Value.isvalid() ? WriteParams(*Value) : true; }
	template<> bool		Write<CParam>(const CParam& Value) { return WriteParam(Value); }
	template<> bool		Write<CData>(const CData& Value) { return WriteData(Value); }
	template<> bool		Write<CDataArray>(const CDataArray& Value);
	template<> bool		Write<PDataArray>(const PDataArray& Value) { return Value.isvalid() ? Write<CDataArray>(*Value) : true; }
	template<> bool		Write<CBuffer>(const CBuffer& Value);
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
		(!Value.Length() || Stream.Write(Value.Get(), Value.Length()) == Value.Length());
}
//---------------------------------------------------------------------

inline bool CBinaryWriter::WriteParams(const CParams& Value)
{
	if (!Write<short>(Value.GetCount())) FAIL;
	for (int i = 0; i < Value.GetCount(); i++)
		if (!WriteParam(Value.Get(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
