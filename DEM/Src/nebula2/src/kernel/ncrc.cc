#include "kernel/ncrc.h"

const uint nCRC::Key = 0x04c11db7;
uint nCRC::Table[256] = { 0 };
bool nCRC::TableInited = false;

nCRC::nCRC()
{
	if (!TableInited)
	{
		for (uint i = 0; i < 256; ++i)
		{
			uint reg = i << 24;
			for (int j = 0; j < 8; ++j)
			{
				bool topBit = (reg & 0x80000000) != 0;
				reg <<= 1;
				if (topBit) reg ^= Key;
			}
			Table[i] = reg;
		}
		TableInited = true;
	}
}
//---------------------------------------------------------------------

uint nCRC::Checksum(uchar* ptr, uint numBytes)
{
	uint reg = 0;
	for (uint i = 0; i < numBytes; i++)
	{
		uint top = reg >> 24;
		top ^= ptr[i];
		reg = (reg << 8) ^ Table[top];
	}
	return reg;
}
//---------------------------------------------------------------------
