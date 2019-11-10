#pragma once
#include <Data.h> 
#include <algorithm> 
#include <fstream>

// Utility functions and data structures

struct membuf : std::streambuf
{
	membuf(char* begin, char* end)
	{
		setg(begin, begin, end);
	}
};

std::vector<std::string> SplitString(const std::string& Str, char Sep);
uint32_t CalcCRC(const uint8_t* pData, size_t Size);

template<class T>
const T& GetParam(const std::map<CStrID, class Data::CData>& Params, const char* pKey, const T& Default)
{
	auto It = Params.find(CStrID(pKey));
	return (It == Params.cend()) ? Default : It->second;
}
//---------------------------------------------------------------------

template<class T>
const bool TryGetParam(T& Out, const std::map<CStrID, class Data::CData>& Params, const char* pKey)
{
	auto It = Params.find(CStrID(pKey));
	if (It == Params.cend() || !It->second.IsA<T>()) return false;
	Out = It->second.GetValue<T>();
	return true;
}
//---------------------------------------------------------------------

template<typename T>
bool ReadAllFile(const char* pPath, std::vector<T>& Out, bool Binary = true)
{
	int Mode = std::ios_base::in;
	if (Binary) Mode |= std::ios_base::binary;
	std::ifstream File(pPath, Mode);
	if (!File) return false;

	Out.assign(std::istreambuf_iterator<T>(File), std::istreambuf_iterator<T>());

	return !File.bad();
}
//---------------------------------------------------------------------

inline bool CompareFloat(float a, float b, float e = std::numeric_limits<float>().epsilon())
{
	return std::fabsf(a - b) <= e;
}
//---------------------------------------------------------------------

inline void EnsurePathHasEndingDirSeparator(std::string& Path)
{
	const size_t PathLen = Path.size();
	if (PathLen && Path[PathLen - 1] != '/') Path += '/';
}
//---------------------------------------------------------------------

template<class T> void ReadStream(std::istream& Stream, T& Out)
{
	Stream.read(reinterpret_cast<char*>(&Out), sizeof(T));
}
//---------------------------------------------------------------------

template<>
inline void ReadStream(std::istream& Stream, std::string& Data)
{
	uint16_t Length;
	ReadStream<uint16_t>(Stream, Length);

	if (Length)
	{
		Data.resize(Length, '\0');
		Stream.read(Data.data(), Length);
	}
}
//---------------------------------------------------------------------

// Can be used to skip data
template<class T> T ReadStream(std::istream& Stream)
{
	T Out;
	ReadStream(Stream, Out);
	return Out;
}
//---------------------------------------------------------------------

template<class T> void WriteStream(std::ostream& Stream, const T& Value)
{
	Stream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const std::string& Value)
{
	const auto Length = static_cast<uint16_t>(Value.size());
	WriteStream<uint16_t>(Stream, Length);
	if (Length) Stream.write(Value.c_str(), Length);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const CStrID& Value)
{
	WriteStream(Stream, std::string(Value.CStr() ? Value.CStr() : ""));
}
//---------------------------------------------------------------------

void WriteData(std::ostream& Stream, const Data::CData& Value);

template<>
inline void WriteStream(std::ostream& Stream, const Data::CData& Value)
{
	WriteData(Stream, Value);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const Data::CDataArray& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const auto& Data : Value)
		WriteStream(Stream, Data);
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const Data::CParams& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const auto& Pair : Value)
	{
		WriteStream(Stream, Pair.first.ToString());
		WriteStream(Stream, Pair.second);
	}
}
//---------------------------------------------------------------------

inline std::string FourCC(uint32_t Code)
{
	std::string Result;
	Result.push_back((Code & 0xff000000) >> 24);
	Result.push_back((Code & 0x00ff0000) >> 16);
	Result.push_back((Code & 0x0000ff00) >> 8);
	Result.push_back(Code & 0x000000ff);
	return Result;
}
//---------------------------------------------------------------------

// trim from start (in place)
inline void ltrim(std::string& s, const std::string& whitespace)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&whitespace](int ch)
	{
		return whitespace.find(ch) == std::string::npos;
	}));
}
//---------------------------------------------------------------------

// trim from end (in place)
inline void rtrim(std::string& s, const std::string& whitespace)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [&whitespace](int ch)
	{
		return whitespace.find(ch) == std::string::npos;
	}).base(), s.end());
}
//---------------------------------------------------------------------

// trim from both ends (in place)
inline void trim(std::string& s, const std::string& whitespace)
{
	ltrim(s, whitespace);
	rtrim(s, whitespace);
}
//---------------------------------------------------------------------

inline void ToLower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}
//---------------------------------------------------------------------

inline void ToUpper(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}
//---------------------------------------------------------------------
