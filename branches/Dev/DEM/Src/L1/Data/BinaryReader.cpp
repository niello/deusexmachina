#include "BinaryReader.h"

#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Data/Buffer.h>

namespace Data
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
	if (SeekOfs > 0) Stream.Seek(SeekOfs, SSO_CURRENT);
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

bool CBinaryReader::ReadString(nString& OutValue)
{
	ushort Len;
	if (!Read(Len)) FAIL;

	if (Len)
	{
		//!!!NEED nString::Reserve!
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

bool CBinaryReader::ReadParams(CParams& OutValue)
{
	//OutValue.Clear();
	short Count;
	if (!Read(Count)) FAIL;
	//!!!OutValue.BeginAdd(Count)!
	for (int i = 0; i < Count; ++i)
	{
		CParam Param;
		if (!ReadParam(Param)) FAIL;
		OutValue.Set(Param);
	}
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadParam(CParam& OutValue)
{
	char Name[256];
	if (!ReadString(Name, sizeof(Name))) FAIL;
	OutValue.SetName(CStrID(Name));
	if (!ReadData(OutValue.GetRawValue())) FAIL;
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadData(CData& OutValue)
{
	char Type;
	if (!Read(Type)) FAIL;

	if (Type == DATA_TYPE_ID(bool)) OutValue = Read<bool>();
	else if (Type == DATA_TYPE_ID(int)) OutValue = Read<int>();
	else if (Type == DATA_TYPE_ID(float)) OutValue = Read<float>();
	else if (Type == DATA_TYPE_ID(nString)) OutValue = Read<nString>();
	else if (Type == DATA_TYPE_ID(CStrID)) OutValue = Read<CStrID>();
	else if (Type == DATA_TYPE_ID(vector4)) OutValue = Read<vector4>();
	else if (Type == DATA_TYPE_ID(matrix44)) OutValue = Read<matrix44>();
	else if (Type == DATA_TYPE_ID(PParams))
	{
		PParams P = n_new(CParams);
		if (!ReadParams(*P)) FAIL;
		OutValue = P;
	}
	else if (Type == DATA_TYPE_ID(PDataArray))
	{
		PDataArray A = n_new(CDataArray);;

		short Count;
		if (!Read(Count)) FAIL;
		A->Reserve(Count);
		for (int i = 0; i < Count; ++i)
			if (!ReadData(A->At(i))) FAIL;

		OutValue = A;
	}
	else if (Type == DATA_TYPE_ID(CBuffer))
	{
		OutValue = CBuffer();
		CBuffer& Buf = OutValue;
		Buf.Reserve(Read<int>());
		Stream.Read(Buf.GetPtr(), Buf.GetSize());
	}
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace Data