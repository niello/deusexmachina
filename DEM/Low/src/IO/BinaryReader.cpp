#include "BinaryReader.h"

#include <Data/DataArray.h>
#include <Data/Buffer.h>
#include <Math/Matrix44.h>

namespace IO
{

bool CBinaryReader::ReadString(char* OutValue, UPTR MaxLen)
{
	U16 Len;
	if (!Read(Len)) FAIL;
	I64 SeekOfs = (I64)Len - (I64)MaxLen + 1;
	if (SeekOfs > 0) Len = (U16)MaxLen - 1;
	if (Stream.Read(OutValue, Len) != Len) FAIL;
	OutValue[Len] = 0;
	if (SeekOfs > 0) Stream.Seek(SeekOfs, IO::Seek_Current);
	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadString(char*& OutValue)
{
	U16 Len;
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

bool CBinaryReader::ReadString(std::string& OutValue)
{
	U16 Len;
	if (!Read(Len)) FAIL;

	if (Len)
	{
		OutValue.resize(Len);
		if (Stream.Read(OutValue.data(), Len) != Len)
		{
			OutValue.clear();
			FAIL;
		}
	}
	else OutValue.clear();

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

	if (Type == INVALID_TYPE_ID) OutValue.Clear();
	else if (Type == DATA_TYPE_ID(bool)) OutValue = Read<bool>();
	else if (Type == DATA_TYPE_ID(int)) OutValue = Read<int>();
	else if (Type == DATA_TYPE_ID(float)) OutValue = Read<float>();
	else if (Type == DATA_TYPE_ID(std::string)) OutValue = Read<std::string>();
	else if (Type == DATA_TYPE_ID(CStrID)) OutValue = Read<CStrID>();
	else if (Type == DATA_TYPE_ID(vector3)) OutValue = Read<vector3>();
	else if (Type == DATA_TYPE_ID(vector4)) OutValue = Read<vector4>();
	else if (Type == DATA_TYPE_ID(matrix44)) OutValue = Read<matrix44>();
	else if (Type == DATA_TYPE_ID(Data::PParams))
	{
		Data::PParams P = n_new(Data::CParams);
		if (!ReadParams(*P)) FAIL;
		OutValue = std::move(P);
	}
	else if (Type == DATA_TYPE_ID(Data::PDataArray))
	{
		Data::PDataArray A = n_new(Data::CDataArray);

		short Count;
		if (!Read(Count)) FAIL;
		if (Count > 0)
		{
			A->resize(Count);
			for (int i = 0; i < Count; ++i)
				if (!ReadData(A->at(i))) FAIL;
		}

		OutValue = std::move(A);
	}
	else if (Type == DATA_TYPE_ID(Data::CBufferMalloc))
	{
		const auto Size = Read<int>();
		Data::CBufferMalloc Buffer(Size);
		Stream.Read(Buffer.GetPtr(), Size);
		OutValue = std::move(Buffer);
	}
	else FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CBinaryReader::ReadDataArray(Data::CDataArray& OutValue)
{
	U16 Count;
	if (!Read<U16>(Count)) FAIL;
	OutValue.resize(Count);
	for (UPTR i = 0; i < Count; ++i)
		if (!ReadData(OutValue[i])) FAIL;
	OK;
}
//---------------------------------------------------------------------

}
