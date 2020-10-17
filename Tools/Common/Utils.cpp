#include "Utils.h"

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
//---------------------------------------------------------------------

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

void WriteData(std::ostream& Stream, const Data::CData& Value)
{
	WriteStream(Stream, static_cast<uint8_t>(Value.GetTypeID()));

	if (Value.IsVoid()) return;
	else if (Value.IsA<bool>()) WriteStream<bool>(Stream, Value);
	else if (Value.IsA<int>()) WriteStream<int>(Stream, Value);
	else if (Value.IsA<float>()) WriteStream<float>(Stream, Value);
	else if (Value.IsA<std::string>()) WriteStream<std::string>(Stream, Value);
	else if (Value.IsA<CStrID>()) WriteStream<CStrID>(Stream, Value);
	else if (Value.IsA<float3>()) WriteStream<float3>(Stream, Value);
	else if (Value.IsA<float4>()) WriteStream<float4>(Stream, Value);
	else if (Value.IsA<Data::CParams>()) WriteStream<Data::CParams>(Stream, Value);
	else if (Value.IsA<Data::CDataArray>()) WriteStream<Data::CDataArray>(Stream, Value);
	//else assert
}
//---------------------------------------------------------------------
