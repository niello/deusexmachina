#include "Utils.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // For the directory chain creation

static const uint32_t CRCKey = 0x04c11db7;
static uint32_t CRCTable[256] = { 0 };
static bool CRCTableInited = false;

uint32_t CalcCRC(const uint8_t* pData, size_t Size)
{
	if (!CRCTableInited)
	{
		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t Reg = i << 24;
			for (int j = 0; j < 8; ++j)
			{
				bool TopBit = (Reg & 0x80000000) != 0;
				Reg <<= 1;
				if (TopBit) Reg ^= CRCKey;
			}
			CRCTable[i] = Reg;
		}
		CRCTableInited = true;
	}

	uint32_t Checksum = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		uint32_t Top = Checksum >> 24;
		Top ^= pData[i];
		Checksum = (Checksum << 8) ^ CRCTable[Top];
	}
	return Checksum;
}
//---------------------------------------------------------------------

// Return a string object containing the part before the last directory separator
std::string ExtractDirName(const std::string& Path)
{
	if (Path.empty()) return {};

	auto Pos = Path.find_last_of("/\\:");

	// Ignore a separator at the last character
	if (Pos == Path.size() - 1)
		Pos = Path.find_last_of("/\\:", Pos - 1);

	if (Pos == std::string::npos) return {};

	return Path.substr(Pos + 1);
}
//---------------------------------------------------------------------

std::string ExtractExtension(const std::string& Path)
{
	const auto DotPos = Path.find_last_of('.');
	if (DotPos == std::string::npos) return {};

	const auto DlmPos = Path.find_last_of(":/\\");
	if (DlmPos != std::string::npos && DlmPos > DotPos) return {};

	return Path.substr(DotPos + 1);
}
//---------------------------------------------------------------------

bool IsPathAbsolute(const std::string& Path)
{
	return Path.find_first_of(':') != std::string::npos;
}
//---------------------------------------------------------------------

bool FileExists(const char* pPath)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && !(FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

bool DirectoryExists(const char* pPath)
{
	DWORD FileAttrs = ::GetFileAttributes(pPath);
	return FileAttrs != INVALID_FILE_ATTRIBUTES && (FileAttrs & FILE_ATTRIBUTE_DIRECTORY);
}
//---------------------------------------------------------------------

bool EnsureDirectoryExists(std::string Path)
{
	std::vector<std::string> DirStack;
	while (!DirectoryExists(Path.c_str()))
	{
		rtrim(Path, " \r\n\t\\/");

		auto LastSepIdx = Path.find_last_of("/\\:");
		if (LastSepIdx != std::string::npos)
		{
			DirStack.push_back(Path.substr(LastSepIdx + 1));
			Path = Path.substr(0, LastSepIdx);
		}
		else
		{
			if (!::CreateDirectory(Path.c_str(), nullptr)) return false;
			break;
		}
	}

	while (DirStack.size())
	{
		Path += '/';
		Path += DirStack.back();
		DirStack.pop_back();
		if (!CreateDirectory(Path.c_str(), nullptr)) return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool ReadAllFile(const char* pPath, std::vector<char>& Out)
{
	std::ifstream File(pPath);
	if (!File) return false;

	Out.assign(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());

	return !File.bad();
}
//---------------------------------------------------------------------

bool EraseFile(const char* pPath)
{
	return ::DeleteFile(pPath) != 0;
}
//---------------------------------------------------------------------

// TODO: compare performance against a naive approach (tokenize folder by folder)
// Returns the same path with '.' (this folder) and '..' (parent folder) collapsed where possible.
// E.g. CollapseDots("One/Two/Three/../../Four") will return "One/Four"
// All '\' slashes in a path must be converted to '/' before calling this function.
std::string CollapseDots(const char* pPath, size_t PathLength)
{
	if (!pPath || !*pPath) return std::string{};

	if (!PathLength) PathLength = strlen(pPath);

	const char* pCurr = pPath;
	const char* pEnd = pPath + PathLength;

	// Adresses of normal directories
	const size_t MAX_DIR_COUNT = 64;
	const char* DirStarts[MAX_DIR_COUNT];
	size_t DirCount = 0;
	size_t ParentCount = 0;

	// Collapsed regions, which will be cut from the resulting path string
	const size_t MAX_CUT_REGION_COUNT = 64;
	struct CCutRegion
	{
		const char*	pStart;
		const char* pEnd;
	};
	CCutRegion CutRegions[MAX_CUT_REGION_COUNT];
	ptrdiff_t CutRegionCount = 0;
	size_t LengthAfterCut = PathLength;

	const char* pDirStart = pCurr;
	while (pCurr <= pEnd)
	{
		char CurrChar = *pCurr;
		if (CurrChar == '/' || CurrChar == ':' || !CurrChar)
		{
			const char* pRegionStart = nullptr;
			const char* pRegionEnd = nullptr;
			size_t DirLength = pCurr - pDirStart;
			if (!DirLength)
			{
				// 'Empty' directory before the beginning or after the ending slash, if present
				if (!CurrChar)
				{
					if (ParentCount)
					{
						pRegionStart = DirStarts[DirCount - ParentCount]; // Skip collected
						pRegionEnd = pCurr;
					}
					else break;
				}
			}
			else if (DirLength == 1 && *pDirStart == '.')
			{
				// '.' directory
				if (!ParentCount)
				{
					pRegionStart = pDirStart;
					//if (!CurrChar && pRegionStart > pPath) --pRegionStart; // Skip ending slash
					pRegionEnd = pCurr;
					if (CurrChar == '/') ++pRegionEnd;
				}
				else if (!CurrChar)
				{
					DirCount -= ParentCount;
					ParentCount = 0;
					pRegionStart = DirStarts[DirCount]; // Skip collected
					if (pRegionStart > pPath) --pRegionStart; // Skip ending slash
					pRegionEnd = pCurr;
					if (CurrChar == '/') ++pRegionEnd;
				}
			}
			else
			{
				bool IsParentDir = (DirLength == 2 && pDirStart[0] == '.' && pDirStart[1] == '.');
				if (IsParentDir && DirCount)
				{
					// '..' directory after at least one normal directory
					++ParentCount;
					if (ParentCount == DirCount || !CurrChar)
					{
						DirCount -= ParentCount;
						ParentCount = 0;
						pRegionStart = DirStarts[DirCount]; // Skip collected
															//if (!CurrChar && pRegionStart > pPath) --pRegionStart; // Skip ending slash
						pRegionEnd = pCurr;
						if (CurrChar == '/') ++pRegionEnd;
					}
				}
				else if (!IsParentDir && CurrChar != ':')
				{
					// Normal directory (not drive letter nor assign)
					if (ParentCount)
					{
						DirCount -= ParentCount;
						ParentCount = 0;
						pRegionStart = DirStarts[DirCount]; // Skip collected
						pRegionEnd = pDirStart;
					}

					DirStarts[DirCount] = pDirStart;
					++DirCount;
					//n_assert(DirCount < MAX_DIR_COUNT);
				}
			}

			if (pRegionStart)
			{
				// Cut region detected, add it
				//n_assert_dbg(pRegionEnd > pRegionStart);

				ptrdiff_t CurrIdx = CutRegionCount - 1;
				while (CurrIdx >= 0)
				{
					CCutRegion& CurrRegion = CutRegions[CurrIdx];

					if (CurrRegion.pStart > pRegionStart)
					{
						// Existing region is consumed by the new one
						--CutRegionCount;
						LengthAfterCut += (CurrRegion.pEnd - CurrRegion.pStart);
					}
					else if (CurrRegion.pEnd == pRegionStart)
					{
						// Adjacent region found, append
						CurrRegion.pEnd = pRegionEnd;
						LengthAfterCut -= (pRegionEnd - pRegionStart);
						break;
					}
					else
					{
						// Exit loop and add new region
						CurrIdx = -1;
						break;
					}

					--CurrIdx;
				}

				if (CurrIdx < 0)
				{
					// New region added
					CutRegions[CutRegionCount].pStart = pRegionStart;
					CutRegions[CutRegionCount].pEnd = pRegionEnd;
					++CutRegionCount;
					//n_assert(CutRegionCount < MAX_CUT_REGION_COUNT);
					LengthAfterCut -= (pRegionEnd - pRegionStart);
				}
			}

			pDirStart = pCurr + 1;
		}

		++pCurr;
	}

	if (!CutRegionCount) return std::string(pPath, PathLength);

	std::string Result;
	Result.reserve(LengthAfterCut);

	ptrdiff_t RegionIdx;
	const char* pStart;
	if (CutRegions[0].pStart == pPath)
	{
		pStart = CutRegions[0].pEnd;
		RegionIdx = 1;
	}
	else
	{
		pStart = pPath;
		RegionIdx = 0;
	}

	for (; RegionIdx < CutRegionCount; ++RegionIdx)
	{
		const CCutRegion& CurrRegion = CutRegions[RegionIdx];
		Result.append(pStart, CurrRegion.pStart - pStart);
		pStart = CurrRegion.pEnd;
	}

	if (pStart < pEnd) Result.append(pStart, pEnd - pStart);

	// Skip ending slash if not present in the original path
	if (!Result.empty() && *(pEnd - 1) != '/' && Result[Result.size() - 1] == '/')
		Result.pop_back();

	return Result;
}
//---------------------------------------------------------------------

//!!!non-optimal, can rewrite in a reverse order to minimize memmove sizes!
// Adds space for each multiline comment stripped to preserve token delimiting in a "name1/*comment*/name2" case
size_t StripComments(char* pStr, const char* pSingleLineComment, const char* pMultiLineCommentStart, const char* pMultiLineCommentEnd)
{
	size_t Len = strlen(pStr);

	if (pMultiLineCommentStart && pMultiLineCommentEnd)
	{
		size_t MLCSLen = strlen(pMultiLineCommentStart);
		size_t MLCELen = strlen(pMultiLineCommentEnd);
		char* pFound;
		while (pFound = strstr(pStr, pMultiLineCommentStart))
		{
			char* pEnd = strstr(pFound + MLCSLen, pMultiLineCommentEnd);
			if (pEnd)
			{
				const char* pFirstValid = pEnd + MLCELen;
				*pFound = ' ';
				++pFound;
				memmove(pFound, pFirstValid, Len - (pFirstValid - pStr));
				Len -= (pFirstValid - pFound);
				pStr[Len] = 0;
			}
			else
			{
				*pFound = 0;
				Len = pFound - pStr;
			}
		}
	}

	if (pSingleLineComment)
	{
		size_t SLCLen = strlen(pSingleLineComment);
		char* pFound;
		while (pFound = strstr(pStr, pSingleLineComment))
		{
			char* pEnd = strpbrk(pFound + SLCLen, "\n\r");
			if (pEnd)
			{
				const char* pFirstValid = pEnd + 1;
				memmove(pFound, pFirstValid, Len - (pFirstValid - pStr));
				Len -= (pFirstValid - pFound);
				pStr[Len] = 0;
			}
			else
			{
				*pFound = 0;
				Len = pFound - pStr;
			}
		}
	}

	return Len;
}
//---------------------------------------------------------------------
