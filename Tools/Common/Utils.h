#pragma once
#include <Data.h> 
#include <algorithm> 
#include <fstream>

// Utility functions

std::vector<std::string> SplitString(const std::string& Str, char Sep);
uint32_t CalcCRC(const uint8_t* pData, size_t Size);
std::string ExtractDirName(const std::string& Path);
std::string ExtractExtension(const std::string& Path);
bool IsPathAbsolute(const std::string& Path);
bool FileExists(const char* pPath);
bool DirectoryExists(const char* pPath);
bool EnsureDirectoryExists(std::string Path);
bool EraseFile(const char* pPath);
std::string CollapseDots(const char* pPath, size_t PathLength = 0);
inline std::string CollapseDots(const std::string& Path) { return CollapseDots(Path.c_str(), Path.size()); }
size_t StripComments(char* pStr, const char* pSingleLineComment = "//", const char* pMultiLineCommentStart = "/*", const char* pMultiLineCommentEnd = "*/");

template<class T>
const T& GetParam(const std::map<CStrID, class Data::CData>& Params, const char* pKey, const T& Default)
{
	auto It = Params.find(CStrID(pKey));
	return (It == Params.cend()) ? Default : It->second;
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

template<class T> void ReadStream(std::istream& Stream, T& Out)
{
	Stream.read(reinterpret_cast<char*>(&Out), sizeof(T));
}
//---------------------------------------------------------------------

// Skip data
template<class T> void ReadStream(std::istream& Stream)
{
	T Out;
	Stream.read(reinterpret_cast<char*>(&Out), sizeof(T));
}
//---------------------------------------------------------------------

template<class T> void WriteStream(std::ostream& Stream, const T& Data)
{
	Stream.write(reinterpret_cast<const char*>(&Data), sizeof(T));
}
//---------------------------------------------------------------------

template<>
inline void WriteStream(std::ostream& Stream, const std::string& Data)
{
	const auto Length = static_cast<uint16_t>(Data.size());
	WriteStream<uint16_t>(Stream, Length);
	Stream.write(Data.c_str(), Length);
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
