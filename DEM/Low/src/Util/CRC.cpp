#include <Util/UtilFwd.h>

// A CRC calculation routine

namespace Util
{
static const UPTR CRCKey = 0x04c11db7;
static UPTR CRCTable[256] = { 0 };
static bool CRCTableInited = false;

UPTR CalcCRC(const U8* pData, UPTR Size)
{
	if (!CRCTableInited)
	{
		for (UPTR i = 0; i < 256; ++i)
		{
			UPTR Reg = i << 24;
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

	UPTR Checksum = 0;
	for (UPTR i = 0; i < Size; ++i)
	{
		UPTR Top = Checksum >> 24;
		Top ^= pData[i];
		Checksum = (Checksum << 8) ^ CRCTable[Top];
	}
	return Checksum;
}
//---------------------------------------------------------------------

}