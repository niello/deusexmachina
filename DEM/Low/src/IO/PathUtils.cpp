#include "PathUtils.h"

namespace PathUtils
{

// TODO: compare performance against a naive approach (tokenize folder by folder)
// Returns the same path with '.' (this folder) and '..' (parent folder) collapsed where possible.
// E.g. CollapseDots("One/Two/Three/../../Four") will return "One/Four"
// All '\' slashes in a path must be converted to '/' before calling this function.
CString CollapseDots(const char* pPath, UPTR PathLength)
{
	if (!pPath || !*pPath) return CString::Empty;

	if (!PathLength) PathLength = strlen(pPath);

	const char* pCurr = pPath;
	const char* pEnd = pPath + PathLength;

	// Adresses of normal directories
	const UPTR MAX_DIR_COUNT = 64;
	const char* DirStarts[MAX_DIR_COUNT];
	UPTR DirCount = 0;
	UPTR ParentCount = 0;

	// Collapsed regions, which will be cut from the resulting path string
	const UPTR MAX_CUT_REGION_COUNT = 64;
	struct CCutRegion
	{
		const char*	pStart;
		const char* pEnd;
	};
	CCutRegion CutRegions[MAX_CUT_REGION_COUNT];
	IPTR CutRegionCount = 0;
	UPTR LengthAfterCut = PathLength;

	const char* pDirStart = pCurr;
	while (pCurr <= pEnd)
	{
		char CurrChar = *pCurr;
		if (CurrChar == '/' || CurrChar == ':' || !CurrChar)
		{
			const char* pRegionStart = nullptr;
			const char* pRegionEnd = nullptr;
			UPTR DirLength = pCurr - pDirStart;
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
					n_assert(DirCount < MAX_DIR_COUNT);
				}
			}

			if (pRegionStart)
			{
				// Cut region detected, add it
				n_assert_dbg(pRegionEnd > pRegionStart);

				IPTR CurrIdx = CutRegionCount - 1;
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
					n_assert(CutRegionCount < MAX_CUT_REGION_COUNT);
					LengthAfterCut -= (pRegionEnd - pRegionStart);
				}
			}
			
			pDirStart = pCurr + 1;
		}

		++pCurr;
	}

	if (!CutRegionCount) return CString(pPath, PathLength);

	CString Result(nullptr, 0, LengthAfterCut);

	IPTR RegionIdx;
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
		Result.Add(pStart, CurrRegion.pStart - pStart);
		pStart = CurrRegion.pEnd;
	}

	if (pStart < pEnd) Result.Add(pStart, pEnd - pStart);

	// Skip ending slash if not present in the original path
	if (Result.GetLength() && *(pEnd - 1) != '/' && Result[Result.GetLength() - 1] == '/')
		Result.TruncateRight(1);

	return Result;
}
//---------------------------------------------------------------------

}