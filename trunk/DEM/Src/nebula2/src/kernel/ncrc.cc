//------------------------------------------------------------------------------
//  ncrc.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "kernel/ncrc.h"

const uint nCRC::key = 0x04c11db7;
uint nCRC::table[NumByteValues] = { 0 };
bool nCRC::tableInitialized = false;

//------------------------------------------------------------------------------
/**
*/
nCRC::nCRC()
{
    // initialize byte table
    if (!tableInitialized)
    {
        uint i;
        for (i = 0; i < NumByteValues; ++i)
        {
            uint reg = i << 24;
            int j;
            for (j = 0; j < 8; ++j)
            {
                bool topBit = (reg & 0x80000000) != 0;
                reg <<= 1;
                if (topBit)
                {
                    reg ^= key;
                }
            }
            table[i] = reg;
        }
        tableInitialized = true;
    }
}

//------------------------------------------------------------------------------
/**
    Compute the checksum for a chunk of memory.
*/
uint
nCRC::Checksum(uchar* ptr, uint numBytes)
{
    uint reg = 0;
    uint i;
    for (i = 0; i < numBytes; i++)
    {
        uint top = reg >> 24;
        top ^= ptr[i];
        reg = (reg << 8) ^ table[top];
    }
    return reg;
}



