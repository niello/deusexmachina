#include "Utils.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // For the directory chain creation

static const uint32_t CRCKey = 0x04c11db7;
static uint32_t CRCTable[256] = { 0 };
static bool CRCTableInited = false;

std::vector<std::string> SplitString(const std::string& Str, char Sep)
{
	std::vector<std::string> result;

	auto i = Str.begin();
	const auto& iEnd = Str.end();
	while (i != iEnd)
	{
		const auto& j = std::find(i, iEnd, Sep);
		result.emplace_back(i, j);
		i = j == iEnd ? iEnd : j + 1;
	}

	return std::move(result);
}

uint32_t CalcCRC(const uint8_t* pData, size_t Size)
{
	if (!CRCTableInited)
	{
		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t Reg = i << 24;
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

	uint32_t Checksum = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		uint32_t Top = Checksum >> 24;
		Top ^= pData[i];
		Checksum = (Checksum << 8) ^ CRCTable[Top];
	}
	return Checksum;
}
//---------------------------------------------------------------------
