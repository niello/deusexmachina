#pragma once
#ifndef __DEM_L1_HASH_H__
#define __DEM_L1_HASH_H__

#include "SuperFastHash.h" //!!!they say Murmur2 is better!
#include <string.h>			// strlen

// Hash functions

inline unsigned int Hash(const void* pData, int Length)
{
	return SuperFastHash((const char*)pData, Length);
}
//---------------------------------------------------------------------

inline unsigned int Hash(const char* pStr)
{
	return Hash(pStr, strlen(pStr));
}
//---------------------------------------------------------------------

template<class T> inline unsigned int Hash(const T& Key)
{
	//???what about smaller than DWORD?
	if (sizeof(T) == sizeof(int)) return WangIntegerHash((int)*(void* const*)&Key);
	else return Hash(&Key, sizeof(T));
}
//---------------------------------------------------------------------

template<> inline unsigned int Hash<const char*>(const char * const & Key)
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

/*
// So called one-at-a-time Hash.
// http://burtleburtle.net/bob/Hash/doobs.html
inline unsigned int OneAtATimeHash(const void* Key, size_t Length)
{
	unsigned int Hash = 0;

	const char* pChar = (const char*)Key;
	const char* pLast = pChar + Length;
	while (pChar != pLast)
	{
		Hash += *pChar;
		Hash += Hash << 10;
		Hash ^= Hash >> 6;
		++pChar;
	}

	Hash += Hash << 3;
	Hash ^= Hash >> 11;
	Hash += Hash << 15;

	return Hash; // & (~(1 << 31)); // Don't return a negative number (in case IndexT is defined signed)
}
//---------------------------------------------------------------------
*/

#endif