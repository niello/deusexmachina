#include "HRDWriter.h"

#include <Data/DataArray.h>
#include <Data/Params.h>
#include <Data/StringUtils.h>

#define WRITE_STATIC_STRING(s) { if (Stream.Write(s, sizeof(s) - 1) != (sizeof(s) - 1)) FAIL; }
#define WRITE_NSTRING(s) { const CString& __Tmp = s; if (Stream.Write(__Tmp.CStr(), __Tmp.GetLength()) != __Tmp.GetLength()) FAIL; }

namespace IO
{

bool CHRDWriter::WriteParams(const Data::CParams& Value)
{
	CurrTabLevel = 0;
	for (UPTR i = 0; i < Value.GetCount(); ++i)
	{
		const Data::CParam& Prm = Value.Get(i);
		if (!WriteParam(Prm)) FAIL;
		if (Prm.IsA<Data::PParams>()) WRITE_STATIC_STRING("\n")
	}
	OK;
}
//---------------------------------------------------------------------

bool CHRDWriter::WriteParam(const Data::CParam& Value)
{
	if (!WriteCharString(Value.GetName().CStr())) FAIL;
	if (Value.IsA<Data::PParams>() || Value.IsA<Data::PDataArray>()) WRITE_STATIC_STRING("\n")
	else WRITE_STATIC_STRING(" = ")
	if (!WriteData(Value.GetRawValue())) FAIL;
	WRITE_STATIC_STRING("\n")
	OK;
}
//---------------------------------------------------------------------

bool CHRDWriter::WriteData(const Data::CData& Value)
{
	if (Value.IsVoid()) WRITE_STATIC_STRING("null")
	else if (Value.IsA<bool>())
	{
		if ((bool)Value) WRITE_STATIC_STRING("true")
		else WRITE_STATIC_STRING("false")
	}
	else if (Value.IsA<int>()) WRITE_NSTRING(StringUtils::FromInt(Value))
	else if (Value.IsA<float>()) WRITE_NSTRING(StringUtils::FromFloat(Value))
	else if (Value.IsA<CString>())
	{
		//!!!correctly serialize \n, \t etc!
		WRITE_STATIC_STRING("\"")
		WRITE_NSTRING(Value.GetValue<CString>())
		WRITE_STATIC_STRING("\"")
	}
	else if (Value.IsA<CStrID>())
	{
		//!!!correctly serialize \n, \t etc!
		WRITE_STATIC_STRING("'")
		if (!WriteCharString(Value.GetValue<CStrID>().CStr())) FAIL;
		WRITE_STATIC_STRING("'")
	}
	else if (Value.IsA<vector3>())
	{
		const vector3& V = Value.GetValue<vector3>();
		WRITE_STATIC_STRING("(")
		WRITE_NSTRING(StringUtils::FromFloat(V.x))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(V.y))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(V.z))
		WRITE_STATIC_STRING(")")
	}
	else if (Value.IsA<vector4>())
	{
		const vector4& V = Value.GetValue<vector4>();
		WRITE_STATIC_STRING("(")
		WRITE_NSTRING(StringUtils::FromFloat(V.x))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(V.y))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(V.z))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(V.w))
		WRITE_STATIC_STRING(")")
	}
	else if (Value.IsA<matrix44>())
	{
		const matrix44& M = Value.GetValue<matrix44>();
		WRITE_STATIC_STRING("(")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[0][0]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[0][1]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[0][2]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[0][3]))
		WRITE_STATIC_STRING(", \n\t")
		if (!WriteIndent()) FAIL;
		WRITE_NSTRING(StringUtils::FromFloat(M.m[1][0]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[1][1]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[1][2]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[1][3]))
		WRITE_STATIC_STRING(", \n\t")
		if (!WriteIndent()) FAIL;
		WRITE_NSTRING(StringUtils::FromFloat(M.m[2][0]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[2][1]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[2][2]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[2][3]))
		WRITE_STATIC_STRING(", \n\t")
		if (!WriteIndent()) FAIL;
		WRITE_NSTRING(StringUtils::FromFloat(M.m[3][0]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[3][1]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[3][2]))
		WRITE_STATIC_STRING(", ")
		WRITE_NSTRING(StringUtils::FromFloat(M.m[3][3]))
		WRITE_STATIC_STRING(")")
	}
	else if (Value.IsA<Data::PDataArray>())
	{
		if (!WriteIndent()) FAIL;
		WRITE_STATIC_STRING("[\n")
		++CurrTabLevel;
		const Data::CDataArray& A = *Value.GetValue<Data::PDataArray>();
		for (UPTR i = 0; i < A.GetCount(); ++i)
		{
			const Data::CData& Elm = A[i];
			if (!Elm.IsA<Data::PParams>() && !Elm.IsA<Data::PDataArray>())
				if (!WriteIndent()) FAIL;
			if (!WriteData(A[i])) FAIL;
			if (i < A.GetCount() - 1) WRITE_STATIC_STRING(",\n")
			else WRITE_STATIC_STRING("\n")
		}
		--CurrTabLevel;
		if (!WriteIndent()) FAIL;
		WRITE_STATIC_STRING("]")
	}
	else if (Value.IsA<Data::PParams>())
	{
		if (!WriteIndent()) FAIL;
		WRITE_STATIC_STRING("{\n")
		++CurrTabLevel;
		Data::PParams P = Value.GetValue<Data::PParams>();
		for (UPTR i = 0; i < P->GetCount(); ++i)
		{
			if (!WriteIndent()) FAIL;
			if (!WriteParam(P->Get(i))) FAIL;
		}
		--CurrTabLevel;
		if (!WriteIndent()) FAIL;
		WRITE_STATIC_STRING("}")
	}
	//else FAIL; //???

	OK;
}
//---------------------------------------------------------------------

bool CHRDWriter::WriteIndent()
{
	for (int i = 0; i < CurrTabLevel; ++i)
		WRITE_STATIC_STRING("\t") //!!!or any other custom indentation sequence!
	OK;
}
//---------------------------------------------------------------------

} //namespace IO