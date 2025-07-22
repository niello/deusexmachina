#pragma once
#include <IO/Stream.h>
#include <Data/Params.h>

// Binary data reader

namespace Data
{
	typedef Ptr<class CDataArray> PDataArray;
}

namespace IO
{

class CBinaryReader final
{
protected:

	IStream& Stream;

public:

	CBinaryReader(IStream& SrcStream) : Stream(SrcStream) {}

	bool				ReadString(char* OutValue, UPTR MaxLen);
	bool				ReadString(char*& OutValue); // Allocates memory
	bool				ReadString(std::string& OutValue);
	bool				ReadParams(Data::CParams& OutValue);
	bool				ReadParam(Data::CParam& OutValue);
	bool				ReadData(Data::CData& OutValue);
	bool				ReadDataArray(Data::CDataArray& OutValue);

	template<class T>
	T					Read() { T Val; n_assert(Read<T>(Val)); return Val; }
	template<class T>
	bool				Read(T& OutValue) { return Stream.Read(&OutValue, sizeof(T)) == sizeof(T); }
	template<> bool		Read<char*>(char*& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<std::string>(std::string& OutValue) { return ReadString(OutValue); }
	template<> bool		Read<CStrID>(CStrID& OutValue);
	template<> bool		Read<Data::CDataArray>(Data::CDataArray& OutValue) { return ReadDataArray(OutValue); }
	template<> bool		Read<Data::PDataArray>(Data::PDataArray& OutValue) { return OutValue.IsValidPtr() ? ReadDataArray(*OutValue) : true; }
	//template<> bool		Read<CParams>(const CParams& OutValue) { return WriteParams(OutValue); }
	//template<> bool		Read<PParams>(const PParams& OutValue) { return OutValue.IsValid() ? Read(*OutValue) : true; }
	//template<> bool		Read<CParam>(const CParam& OutValue) { return WriteParam(OutValue); }
	template<> bool		Read<Data::CData>(Data::CData& OutValue) { return ReadData(OutValue); }

	/*
	template<typename U>
	bool                Read<std::optional<U>>(std::optional<U>& OutValue)
	{
		bool HasValue = false;
		if (!Read(HasValue)) return false;
		if (HasValue)
		{
			U NewValue{};
			if (!Read(NewValue)) return false;
			OutValue = std::move(NewValue);
		}
		else
		{
			OutValue.reset();
		}
		return true;
	}
	*/

	template<class T>
	CBinaryReader& operator >>(T& OutValue) { Read(OutValue); return *this; }

	IStream& GetStream() const { return Stream; }
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
