#ifndef N_CRC_H
#define N_CRC_H

#include "kernel/ntypes.h"

// A CRC checker class.
// (C) 2004 RadonLabs GmbH

class nCRC
{
private:

	static const uint	Key;
	static uint			Table[256]; // MUST BE 256 (for each possible byte value 1 entry)
	static bool			TableInited;

public:

	nCRC();

	uint Checksum(uchar* ptr, uint numBytes);
};

#endif
