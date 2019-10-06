#pragma once
#include <RenderState.h>
#include <Data.h>
#include <set>

struct CTechnique
{
	std::vector<CStrID> Passes;

	CStrID ID;
	CStrID InputSet;

	uint32_t ShaderFormatFourCC = 0;
	uint32_t MinFeatureLevel = 0;

	bool operator <(const CTechnique& Other)
	{
		// NB: not sorted by shader format, grouped by it in a map instead

		// Sort by input set to group all related techs and make selection easier
		if (InputSet != Other.InputSet) return InputSet < Other.InputSet;

		// Sort by features descending, so we start with the most interesting
		// tech and fall back to simpler ones
		return MinFeatureLevel > Other.MinFeatureLevel;
	}
};

#pragma pack(push, 1)
struct CShaderHeader
{
	uint32_t Format;
	uint32_t MinFeatureLevel;
	uint8_t Type;
	uint32_t BinaryOffset;
};
#pragma pack(pop)

struct CShaderData
{
	CShaderHeader Header;
	std::unique_ptr<char[]> MetaBytes;
	size_t MetaByteCount;
};

struct CContext
{
	std::set<CStrID> GlobalParams;
	Data::CParams MaterialParams;
	std::map<CStrID, CRenderState> RSCache;
	std::map<CStrID, CShaderData> ShaderCache;
	std::map<uint32_t, std::vector<CTechnique>> TechsByFormat; // Grouped by shader format
	int LogVerbosity; // TODO: logger instance instead!
	char LineEnd; // TODO: logger instance instead!
};
