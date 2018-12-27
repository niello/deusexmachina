#ifndef N_CRC_H
#define N_CRC_H

#include <StdDEM.h>

// Utility algorithms

namespace Util
{
	UPTR	CalcCRC(const U8* pData, UPTR Size);
	float	GeneratePerlinNoise(float x, float y, float z);
}

#endif
