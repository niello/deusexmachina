#pragma once
#include <algorithm> 
#include <string> 
#include <vector> 
#include <fstream>

// Utility functions

uint32_t CalcCRC(const uint8_t* pData, size_t Size);
std::string ExtractDirName(const std::string& Path);
bool DirectoryExists(const char* pPath);
bool EnsureDirectoryExists(std::string Path);
bool EraseFile(const char* pPath);
std::string CollapseDots(const char* pPath, size_t PathLength = 0);
inline std::string CollapseDots(const std::string& Path) { return CollapseDots(Path.c_str(), Path.size()); }
size_t StripComments(char* pStr, const char* pSingleLineComment = "//", const char* pMultiLineCommentStart = "/*", const char* pMultiLineCommentEnd = "*/");

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
