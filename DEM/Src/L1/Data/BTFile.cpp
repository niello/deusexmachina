#include "BTFile.h"

#include <float.h>
#include <memory.h>

namespace Data
{
const float CBTFile::NoDataF = -FLT_MAX;
const short CBTFile::NoDataS = -32768;

CBTFile::CBTFile(const void* Data): SelfAllocated(false), Version(BT_INVALID), MinHeight(NoDataF), MaxHeight(NoDataF)
{
	n_assert2(Data, "NULL data in constructor of CBTFile");
	Header = (CHeaderUnion*)Data;
	HeightsF = (float*)((char*)Data + sizeof(CHeaderUnion));
	InitVersion();
	n_assert(Version != BT_INVALID);
}
//---------------------------------------------------------------------

void CBTFile::InitVersion()
{
	if (!memcmp(Header->ID, "binterr1.", 9))
		switch (Header->ID[9])
		{
			case '1': Version = BT_V_1_1; return;
			case '2': Version = BT_V_1_2; return;
			case '3': Version = BT_V_1_3; return;
		}
	Version = BT_INVALID;
}
//---------------------------------------------------------------------

float CBTFile::GetMinHeight()
{
	n_assert(Header);
	DWORD HCount = GetHeightCount();
	if (!HCount) return NoDataF;

	if (MinHeight == NoDataF)
	{
		if (IsFloatData())
		{
			MinHeight = *HeightsF;
			float* CurrHeight = HeightsF;
			float* HeightStop = HeightsF + HCount;
			while (++CurrHeight < HeightStop)
				if (MinHeight > *CurrHeight && *CurrHeight != NoDataF) MinHeight = *CurrHeight;
		}
		else
		{
			short MinHeightS = *HeightsS;
			short* CurrHeight = HeightsS;
			short* HeightStop = HeightsS + HCount;
			while (++CurrHeight < HeightStop)
				if (MinHeightS > *CurrHeight && *CurrHeight != NoDataS) MinHeightS = *CurrHeight;
			MinHeight = (MinHeightS == NoDataS) ? NoDataF : MinHeightS * GetVerticalScale();
		}

		n_assert2(MinHeight != NoDataF, "Completely empty heightfield");
	}

	return MinHeight;
}
//---------------------------------------------------------------------

float CBTFile::GetMaxHeight()
{
	n_assert(Header);
	DWORD HCount = GetHeightCount();
	if (!HCount) return NoDataF;

	if (MaxHeight == NoDataF)
	{
		if (IsFloatData())
		{
			MaxHeight = *HeightsF;
			float* CurrHeight = HeightsF;
			float* HeightStop = HeightsF + HCount;
			while (++CurrHeight < HeightStop)
				if (MaxHeight < *CurrHeight) MaxHeight = *CurrHeight;
		}
		else
		{
			short MaxHeightS = *HeightsS;
			short* CurrHeight = HeightsS;
			short* HeightStop = HeightsS + HCount;
			while (++CurrHeight < HeightStop)
				if (MaxHeightS < *CurrHeight) MaxHeightS = *CurrHeight;
			MaxHeight = (MaxHeightS == NoDataS) ? NoDataF : MaxHeightS * GetVerticalScale();
		}

		n_assert2(MaxHeight != NoDataF, "Completely empty heightfield");
	}

	return MaxHeight;
}
//---------------------------------------------------------------------

float CBTFile::GetHeight(DWORD X, DWORD Z) const
{
	n_assert(Header);
	n_assert(X < Header->Width);
	n_assert(Z < Header->Height);
	if (IsFloatData()) return GetHeightF(X, Z);
	else
	{
		short Height = GetHeightS(X, Z);
		return (Height == NoDataS) ? NoDataF : Height * GetVerticalScale();
	}
}
//---------------------------------------------------------------------

// OutFloats must be preallocated
// BT stores south-to-north column-major, but we return N-to-S row-major here
void CBTFile::GetHeights(float* OutFloats, DWORD X, DWORD Z, DWORD W, DWORD H)
{
	n_assert(Header);
	n_assert(OutFloats);
	n_assert(X < Header->Width);
	n_assert(Z < Header->Height);
	if (W == 0) W = Header->Width - X;
	else n_assert(X + W < Header->Width);
	if (H == 0) H = Header->Height - Z;
	else n_assert(Z + H < Header->Height);

	float* Dest = OutFloats;
	
	if (IsFloatData())
	{
		n_assert(Header->DataSize == sizeof(float));
		for (DWORD Row = Z; Row < Z + H; Row++)
			for (DWORD Col = X; Col < X + W; Col++)
				Dest[Row * Header->Width + Col] =
					HeightsF[Col * Header->Height + Header->Height - 1 - Row];
	}
	else
	{
		n_assert(Header->DataSize == sizeof(short));
		float VScale = GetVerticalScale();
		for (DWORD Row = Z; Row < Z + H; Row++)
			for (DWORD Col = X; Col < X + W; Col++)
			{
				short Src = HeightsS[Col * Header->Height + Header->Height - 1 - Row];
				Dest[Row * Header->Width + Col] = (Src == NoDataS) ? NoDataF : Src * VScale;
			}
    }
}
//---------------------------------------------------------------------

}