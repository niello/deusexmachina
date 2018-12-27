#pragma once
#ifndef __DEM_L1_ARRAY_UTILS_H__
#define __DEM_L1_ARRAY_UTILS_H__

#include <StdDEM.h>

// Utilities for arrays

namespace ArrayUtils
{

//!!!UPTR, invalid index (UPTR)(-1)!
template<class T>
IPTR FindIndexSorted(const T* pData, UPTR Count, const T& Val)
{
	if (!pData || !Count) return INVALID_INDEX;

	IPTR Idx = &Val - pData;
	if ((UPTR)Idx < Count) return Idx;

	IPTR Num = Count, Low = 0, High = Num - 1;
	while (Low <= High)
	{
		IPTR Half = (Num >> 1);
		if (Half)
		{
			IPTR Mid = Low + Half - 1 + (Num & 1);
			if (Val < pData[Mid])
			{
				High = Mid - 1;
				Num = Mid - Low;
			}
			else if (Val > pData[Mid])
			{
				Low = Mid + 1;
				Num = Half;
			}
			else return Mid;
		}
		else
		{
			n_assert_dbg(Num);
			return (Val == pData[Low]) ? Low : INVALID_INDEX;
		}
	}

	return INVALID_INDEX;
}
//---------------------------------------------------------------------

//!!!UPTR!
// Returns where this element should be inserted to keep array sorted
template<class T>
IPTR FindClosestIndexSorted(const T* pData, UPTR Count, const T& Val, bool* pHasEqualElement)
{
	if (!pData || !Count)
	{
		if (pHasEqualElement) *pHasEqualElement = false;
		return 0;
	}

	IPTR Num = Count, Low = 0, High = Num - 1;
	while (Low <= High)
	{
		IPTR Half = (Num >> 1);
		if (Half)
		{
			IPTR Mid = Low + Half - 1 + (Num & 1);
			if (Val < pData[Mid])
			{
				High = Mid - 1;
				Num = Mid - Low;
			}
			else if (Val > pData[Mid])
			{
				Low = Mid + 1;
				Num = Half;
			}
			else
			{
				if (pHasEqualElement) *pHasEqualElement = true;
				return Mid + 1;
			}
		}
		else
		{
			if (pHasEqualElement)
			{
				// The only place where == is required, may rewrite through >, but it is less optimal (see CHashPairT)
				bool IsEqual = (Val == pData[Low]);
				*pHasEqualElement = IsEqual;
				if (IsEqual) return Low + 1;
			}
			return (Val < pData[Low]) ? Low : Low + 1;
		}
	}

	if (pHasEqualElement) *pHasEqualElement = false;
	return Low;
}
//---------------------------------------------------------------------

}

#endif
