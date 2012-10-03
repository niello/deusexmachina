#ifndef N_CRC_H
#define N_CRC_H
//------------------------------------------------------------------------------
/**
    @class nCRC
    @ingroup Kernel

    A CRC checker class.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
class nCRC
{
public:
    /// default constructor
    nCRC();
    /// compute a CRC checksum for a chunk of memory
    uint Checksum(uchar* ptr, uint numBytes);

private:
    enum
    {
        NumByteValues = 256,      // MUST BE 256 (for each possible byte value 1 entry)
    };
    static const uint key;
    static uint table[NumByteValues];
    static bool tableInitialized;
};
//------------------------------------------------------------------------------
#endif
