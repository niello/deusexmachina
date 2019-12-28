#include "BTFile.h"
#include <cassert>
#include <cstring>

CBTFile::CBTFile(const void* Data)
	: Version(BT_INVALID)
	, MinHeight(NoDataF)
	, MaxHeight(NoDataF)
{
	Header = static_cast<const CHeaderUnion*>(Data);
	HeightsF = reinterpret_cast<const float*>((const char*)Data + sizeof(CHeaderUnion));
	InitVersion();
	assert(Version != BT_INVALID);
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
	assert(Header);
	size_t HCount = GetHeightCount();
	if (!HCount) return NoDataF;

	if (MinHeight == NoDataF)
	{
		if (IsFloatData())
		{
			MinHeight = *HeightsF;
			const float* CurrHeight = HeightsF;
			const float* HeightStop = HeightsF + HCount;
			while (++CurrHeight < HeightStop)
				if (MinHeight > *CurrHeight && *CurrHeight != NoDataF) MinHeight = *CurrHeight;
		}
		else
		{
			short MinHeightS = *HeightsS;
			const short* CurrHeight = HeightsS;
			const short* HeightStop = HeightsS + HCount;
			while (++CurrHeight < HeightStop)
				if (MinHeightS > *CurrHeight && *CurrHeight != NoDataS) MinHeightS = *CurrHeight;
			MinHeight = (MinHeightS == NoDataS) ? NoDataF : MinHeightS * GetVerticalScale();
		}

		assert((MinHeight != NoDataF) && "Completely empty heightfield");
	}

	return MinHeight;
}
//---------------------------------------------------------------------

float CBTFile::GetMaxHeight()
{
	assert(Header);
	size_t HCount = GetHeightCount();
	if (!HCount) return NoDataF;

	if (MaxHeight == NoDataF)
	{
		if (IsFloatData())
		{
			MaxHeight = *HeightsF;
			const float* CurrHeight = HeightsF;
			const float* HeightStop = HeightsF + HCount;
			while (++CurrHeight < HeightStop)
				if (MaxHeight < *CurrHeight) MaxHeight = *CurrHeight;
		}
		else
		{
			short MaxHeightS = *HeightsS;
			const short* CurrHeight = HeightsS;
			const short* HeightStop = HeightsS + HCount;
			while (++CurrHeight < HeightStop)
				if (MaxHeightS < *CurrHeight) MaxHeightS = *CurrHeight;
			MaxHeight = (MaxHeightS == NoDataS) ? NoDataF : MaxHeightS * GetVerticalScale();
		}

		assert((MaxHeight != NoDataF) && "Completely empty heightfield");
	}

	return MaxHeight;
}
//---------------------------------------------------------------------

float CBTFile::GetHeight(size_t X, size_t Z) const
{
	assert(Header);
	assert(X < Header->Width);
	assert(Z < Header->Height);
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
void CBTFile::GetHeights(float* OutFloats, size_t X, size_t Z, size_t W, size_t H)
{
	assert(Header);
	assert(OutFloats);
	assert(X < Header->Width);
	assert(Z < Header->Height);
	if (W == 0) W = Header->Width - X;
	else assert(X + W < Header->Width);
	if (H == 0) H = Header->Height - Z;
	else assert(Z + H < Header->Height);

	float* Dest = OutFloats;
	
	if (IsFloatData())
	{
		assert(Header->DataSize == sizeof(float));
		for (size_t Row = Z; Row < Z + H; ++Row)
			for (size_t Col = X; Col < X + W; ++Col)
				Dest[Row * Header->Width + Col] =
					HeightsF[Col * Header->Height + Header->Height - 1 - Row];
	}
	else
	{
		assert(Header->DataSize == sizeof(short));
		float VScale = GetVerticalScale();
		for (size_t Row = Z; Row < Z + H; ++Row)
			for (size_t Col = X; Col < X + W; ++Col)
			{
				short Src = HeightsS[Col * Header->Height + Header->Height - 1 - Row];
				Dest[Row * Header->Width + Col] = (Src == NoDataS) ? NoDataF : Src * VScale;
			}
    }
}
//---------------------------------------------------------------------
