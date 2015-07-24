#include "BinaryReader.h"

#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Data/Buffer.h>
#include <Math/Matrix44.h>

namespace IO
{

bool CBinaryReader::ReadString(char* OutValue, DWORD MaxLen)
{
	ushort Len;
	int SeekOfs;
	if (!Read(Len)) FAIL;
	SeekOfs = (DWORD)Len - (MaxLen - 1);
	if (SeekOfs > 0) Len = (ushort)(MaxLen - 1);
	if (Stream.Read(OutValue, Len) != Len) FAIL;
	OutValue[Len] = 0;
	if (SeekOfs > 0) Stream.Seek(SeekOfs, IO::Seek_Current);
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadString(char*& OutValue)
{
	ushort Len;
	if (!Read(Len)) FAIL;
	OutValue = n_new_array(char, Len + 1);
	if (Stream.Read(OutValue, Len) != Len)
	{
		n_delete_array(OutValue);
		FAIL;
	}
	OutValue[Len] = 0;
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadString(CString& OutValue)
{
	ushort Len;
	if (!Read(Len)) FAIL;

	if (Len)
	{
		//!!!NEED CString::Reserve!
		//OutValue.Reserve(Len);

		char* pTmp = n_new_array(char, Len + 1);
		if (Stream.Read(pTmp, Len) != Len)
		{
			n_delete_array(pTmp);
			FAIL;
		}
		pTmp[Len] = 0;
		OutValue.Set(pTmp, Len);
		n_delete_array(pTmp);
	}
	else OutValue.Set("");

	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadParams(Data::CParams& OutValue)
{
	n_assert_dbg(!OutValue.GetCount());
	short Count;
	if (!Read(Count)) FAIL;
	//!!!OutValue.BeginAdd(Count)!
	for (int i = 0; i < Count; ++i)
	{
		Data::CParam Param;
		if (!ReadParam(Param)) FAIL;
		OutValue.Set(Param);
	}
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadParam(Data::CParam& OutValue)
{
	char Name[256];
	if (!ReadString(Name, sizeof(Name))) FAIL;
	OutValue.SetName(CStrID(Name));
	if (!ReadData(OutValue.GetRawValue())) FAIL;
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadData(Data::CData& OutValue)
{
	char Type;
	if (!Read(Type)) FAIL;

	if (Type == TVoid) OutValue.Clear();
	else if (Type == DATA_TYPE_ID(bool)) OutValue = Read<bool>();
	else if (Type == DATA_TYPE_ID(int)) OutValue = Read<int>();
	else if (Type == DATA_TYPE_ID(float)) OutValue = Read<float>();
	else if (Type == DATA_TYPE_ID(CString)) OutValue = Read<CString>();
	else if (Type == DATA_TYPE_ID(CStrID)) OutValue = Read<CStrID>();
	else if (Type == DATA_TYPE_ID(vector3)) OutValue = Read<vector3>();
	else if (Type == DATA_TYPE_ID(vector4)) OutValue = Read<vector4>();
	else if (Type == DATA_TYPE_ID(matrix44)) OutValue = Read<matrix44>();
	else if (Type == DATA_TYPE_ID(Data::PParams))
	{
		Data::PParams P = n_new(Data::CParams);
		if (!ReadParams(*P)) FAIL;
		OutValue = P;
	}
	else if (Type == DATA_TYPE_ID(Data::PDataArray))
	{
		Data::PDataArray A = n_new(Data::CDataArray);

		short Count;
		if (!Read(Count)) FAIL;
		if (Count > 0)
		{
			A->Reserve(Count);
			for (int i = 0; i < Count; ++i)
				if (!ReadData(A->At(i))) FAIL;
		}

		OutValue = A;
	}
	else if (Type == DATA_TYPE_ID(Data::CBuffer))
	{
		OutValue = Data::CBuffer();
		Data::CBuffer& Buf = OutValue;
		Buf.Reserve(Read<int>());
		Stream.Read(Buf.GetPtr(), Buf.GetSize());
	}
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace IO