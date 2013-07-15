#include <Util/UtilFwd.h>

// A CRC calculation routine

namespace Util
{
static const uint CRCKey = 0x04c11db7;
static uint CRCTable[256] = { 0 };
static bool CRCTableInited = false;

uint CalcCRC(const uchar* pData, uint Size)
{
	if (!CRCTableInited)
	{
		for (uint i = 0; i < 256; ++i)
		{
			uint Reg = i << 24;
			for (int j = 0; j < 8; ++j)
			{
				bool TopBit = (Reg & 0x80000000) != 0;
				Reg <<= 1;
				if (TopBit) Reg ^= CRCKey;
			}
			CRCTable[i] = Reg;
		}
		CRCTableInited = true;
	}

	uint Checksum = 0;
	for (uint i = 0; i < Size; ++i)
	{
		uint Top = Checksum >> 24;
		Top ^= pData[i];
		Checksum = (Checksum << 8) ^ CRCTable[Top];
	}
	return Checksum;
}
//---------------------------------------------------------------------

}