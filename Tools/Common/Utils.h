#pragma once
#include <Data.h> 
#include <algorithm> 
#include <fstream>
#include <vector>

// Utility functions and data structures

struct membuf : std::streambuf
{
	membuf(char* begin, char* end)
	{
		setg(begin, begin, end);
	}
};

// Code from https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
template <typename T>
void apply_permutation_in_place(std::vector<T>& vec, const std::vector<size_t>& p)
{
	std::vector<bool> done(vec.size());
	for (size_t i = 0; i < vec.size(); ++i)
	{
		if (done[i]) continue;
		done[i] = true;
		size_t prev_j = i;
		size_t j = p[i];
		while (i != j)
		{
			std::swap(vec[prev_j], vec[j]);
			done[j] = true;
			prev_j = j;
			j = p[j];
		}
	}
}
//---------------------------------------------------------------------

template <typename T>
void apply_permutation(std::vector<T>& vec, const std::vector<size_t>& p)
{
	std::vector<T> sorted_vec(vec.size());
	std::transform(p.begin(), p.end(), sorted_vec.begin(), [&vec](size_t i) { return vec[i]; });
	std::swap(vec, sorted_vec);
}
//---------------------------------------------------------------------

std::vector<std::string> SplitString(const std::string& Str, char Sep);
uint32_t CalcCRC(const uint8_t* pData, size_t Size);

inline float ByteToNormalizedFloat(uint8_t value)
{
	return value / 255.0f;
}
//---------------------------------------------------------------------

inline uint8_t NormalizedFloatToByte(float value)
{
	return static_cast<uint8_t>(value * 255.0f + 0.5f);
}
//---------------------------------------------------------------------

inline float ShortToNormalizedFloat(uint16_t value)
{
	return value / 65535.0f;
}
//---------------------------------------------------------------------

inline uint16_t NormalizedFloatToShort(float value)
{
	return static_cast<uint16_t>(value * 65535.0f + 0.5f);
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

inline void EnsurePathHasEndingDirSeparator(std::string& Path)
{
	const size_t PathLen = Path.size();
	if (PathLen && Path[PathLen - 1] != '/') Path += '/';
}
//---------------------------------------------------------------------

template<class T> inline void ReadStream(std::istream& Stream, T& Out)
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
template<class T> inline T ReadStream(std::istream& Stream)
{
	T Out;
	ReadStream(Stream, Out);
	return Out;
}
//---------------------------------------------------------------------

template<class T> inline void WriteStream(std::ostream& Stream, const T& Value)
{
	Stream.write(reinterpret_cast<const char*>(&Value), sizeof(T));
}
//---------------------------------------------------------------------

inline void WriteText(std::ostream& Stream, const std::string& Value)
{
	Stream.write(Value.c_str(), Value.size());
}
//---------------------------------------------------------------------

inline void WriteText(std::ostream& Stream, const char* Value)
{
	if (Value) Stream.write(Value, strlen(Value));
}
//---------------------------------------------------------------------

template<size_t N>
inline void WriteText(std::ostream& Stream, const char (&Value)[N])
{
	Stream.write(Value, N - 1);
}
//---------------------------------------------------------------------

template<size_t N>
void WriteCharArrayToStream(std::ostream& Stream, const char (&Value)[N])
{
	auto Length = static_cast<uint16_t>(N);
	if (Value[N - 1] == 0) --Length;
	WriteStream<uint16_t>(Stream, Length);
	if (Length) Stream.write(Value, Length);
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

template<typename T>
inline void WriteVectorToStream(std::ostream& Stream, const std::vector<T>& Value)
{
	WriteStream(Stream, static_cast<uint16_t>(Value.size()));
	for (const T& Data : Value)
		WriteStream<T>(Stream, Data);
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

inline std::string GetValidFileName(const std::string& BaseName)
{
	// TODO: std::transform with replacer callback?
	std::string RsrcName = BaseName;
	std::replace(RsrcName.begin(), RsrcName.end(), ':', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '|', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '?', '_');
	return RsrcName;
}
//---------------------------------------------------------------------

inline std::string GetValidResourceName(const std::string& BaseName)
{
	std::string RsrcName = GetValidFileName(BaseName);
	std::replace(RsrcName.begin(), RsrcName.end(), ' ', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '-', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '.', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '^', '_');
	ToLower(RsrcName);
	return RsrcName;
}
//---------------------------------------------------------------------

inline std::string GetValidNodeName(const std::string& BaseName)
{
	std::string RsrcName = GetValidFileName(BaseName);
	std::replace(RsrcName.begin(), RsrcName.end(), '.', '_');
	std::replace(RsrcName.begin(), RsrcName.end(), '^', '_');
	return RsrcName;
}
//---------------------------------------------------------------------
