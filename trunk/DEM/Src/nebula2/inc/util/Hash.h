#pragma once
#ifndef __DEM_L1_HASH_H__
#define __DEM_L1_HASH_H__

// Hash functions

template<class T> inline unsigned int Hash(const T& Key)
{
	//???what about smaller then DWORD?
	if (sizeof(T) == sizeof(int)) return WangIntegerHash((int)*(void* const*)&Key);
	else return OneAtATimeHash(&Key, sizeof(T));
}
//---------------------------------------------------------------------

inline int WangIntegerHash(int Key)
{
	Key = ~Key + (Key << 15); // key = (key << 15) - key - 1;
	Key = Key ^ (Key >> 12);
	Key = Key + (Key << 2);
	Key = Key ^ (Key >> 4);
	Key = Key * 2057; // key = (key + (key << 3)) + (key << 11);
	Key = Key ^ (Key >> 16);
	return Key;
}
//---------------------------------------------------------------------

//!!!
/**
    So called one-at-a-time Hash.
    @ref http://burtleburtle.net/bob/Hash/doobs.html
    @todo Implement SuperFastHash from http://www.azillionmonkeys.com/qed/Hash.html
*/
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

#endif