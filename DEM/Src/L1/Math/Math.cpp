#include "Math.h"

#include <time.h>

namespace Math
{

static U32 WELL512_State[16];
static U32 WELL512_Index = 0;

// Code from http://lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
inline U32 RunWELL512()
{
	U32 a, b, c, d;
	a = WELL512_State[WELL512_Index];
	c = WELL512_State[(WELL512_Index+13)&15];
	b = a^c^(a<<16)^(c<<15);
	c = WELL512_State[(WELL512_Index+9)&15];
	c ^= (c>>11);
	a = WELL512_State[WELL512_Index] = b^c;
	d = a^((a<<5)&0xDA442D24UL);
	WELL512_Index = (WELL512_Index + 15)&15;
	a = WELL512_State[WELL512_Index];
	WELL512_State[WELL512_Index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
	return WELL512_State[WELL512_Index];
}
//---------------------------------------------------------------------

void InitRandomNumberGenerator()
{
	srand((unsigned int)time(NULL));

	// For WELL512 we must initialize 16 x U32 values, but rand() gives us values
	// between 0 and 32767, which is a half of a short.

	for (U32 i = 0; i < 16; ++i)
	{
		U32 Low = rand() + rand();
		U32 High = rand() + rand();
		WELL512_State[i] = (High << 16) | Low;
	}

	WELL512_Index = 0;
}
//---------------------------------------------------------------------

U32 RandomU32()
{
	return RunWELL512();
}
//---------------------------------------------------------------------

}
