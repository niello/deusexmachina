#pragma once
#include "MurmurHash3.h"
#include <string.h>			// strlen

// Hash functions

inline uint32_t Hash(const void* pData, int Length)
{
	if (!pData || !Length) return 0;

	uint32_t Result;
	MurmurHash3_x86_32(pData, Length, 0xB0F57EE3, &Result);
	return Result;
}
//---------------------------------------------------------------------

inline uint32_t Hash(const char* pStr)
{
	return pStr ? Hash(pStr, strlen(pStr)) : 0;
}
//---------------------------------------------------------------------

template<class T> inline uint32_t Hash(const T& Key)
{
	//???what about smaller than int? what about x64 pointers?
	if (sizeof(T) == sizeof(int)) return WangIntegerHash((int)*(void* const*)&Key);
	else return Hash(&Key, sizeof(T));
}
//---------------------------------------------------------------------

template<> inline uint32_t Hash<const char*>(const char * const & Key)
{
	return Hash(Key, strlen(Key));
}
//---------------------------------------------------------------------

inline int WangIntegerHash(int Key)
{
	//if (sizeof(void*)==8) RealKey = Key[0] + Key[1];

	// Don't remember where I found it
	//Key = ~Key + (Key << 15); // Key = (Key << 15) - Key - 1;
	//Key = Key ^ (Key >> 12);
	//Key = Key + (Key << 2);
	//Key = Key ^ (Key >> 4);
	//Key = Key * 2057; // Key = (Key + (Key << 3)) + (Key << 11);
	//Key = Key ^ (Key >> 16);

	// Bullet Physics variant
	Key += ~(Key << 15);
	Key ^=  (Key >> 10);
	Key +=  (Key << 3);
	Key ^=  (Key >> 6);
	Key += ~(Key << 11);
	Key ^=  (Key >> 16);
	return Key;
}
//---------------------------------------------------------------------
