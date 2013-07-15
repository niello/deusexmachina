#ifndef N_CRC_H
#define N_CRC_H

#include <kernel/ntypes.h>

// A CRC calculation routine
// (C) 2004 RadonLabs GmbH

namespace Util
{
	uint CalcCRC(const uchar* pData, uint Size);
}

#endif
