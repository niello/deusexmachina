#ifndef N_CRC_H
#define N_CRC_H

#include <StdDEM.h>

// Utility algorithms

namespace Util
{
	uint	CalcCRC(const uchar* pData, uint Size);
	float	GeneratePerlinNoise(float x, float y, float z);
}

#endif
