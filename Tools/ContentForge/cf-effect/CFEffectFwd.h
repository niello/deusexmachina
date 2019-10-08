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

template<class TBufferMeta>
void CopyBufferMetadata(uint32_t& BufferIndex, const std::vector<TBufferMeta>& SrcBuffers, std::vector<TBufferMeta>& TargetBuffers)
{
	if (BufferIndex == static_cast<uint32_t>(-1)) return;

	const auto& Buffer = SrcBuffers[BufferIndex];
	auto ItBuffer = std::find(TargetBuffers.cbegin(), TargetBuffers.cend(), Buffer);
	if (ItBuffer != TargetBuffers.cend())
	{
		// The same buffer found, reference it
		BufferIndex = static_cast<uint32_t>(std::distance(TargetBuffers.cbegin(), ItBuffer));
	}
	else
	{
		// Copy new buffer to metadata
		TargetBuffers.push_back(Buffer);
		BufferIndex = static_cast<uint32_t>(TargetBuffers.size() - 1);
	}
}
//---------------------------------------------------------------------

template<class TStructMeta>
void CopyStructMetadata(uint32_t& StructIndex, const std::vector<TStructMeta>& SrcStructs, std::vector<TStructMeta>& TargetStructs)
{
	if (StructIndex == static_cast<uint32_t>(-1) || SrcStructs[StructIndex].Members.empty()) return;

	// Copy the structure
	auto Struct = SrcStructs[StructIndex];

	// Recursively process metadata
	for (auto& Member : Struct.Members)
		CopyStructMetadata(Member.StructIndex, SrcStructs, TargetStructs);

	auto ItStruct = std::find_if(TargetStructs.cbegin(), TargetStructs.cend(), [&Struct](const TStructMeta& OtherStruct)
	{
		if (Struct.Members.size() != OtherStruct.Members.size()) return false;

		// Compare including child order, because we search for exact copies and don't expect sudden coincidences
		for (size_t i = 0; i < Struct.Members.size(); ++i)
		{
			const auto& StructMember = Struct.Members[i];
			const auto& OtherStructMember = OtherStruct.Members[i];
			if (StructMember != OtherStructMember || StructMember.StructIndex != OtherStructMember.StructIndex) return false;
		}

		return true;
	});

	if (ItStruct != TargetStructs.cend())
	{
		// The same struct found, reference it
		StructIndex = static_cast<uint32_t>(std::distance(TargetStructs.cbegin(), ItStruct));
	}
	else
	{
		// Copy new struct to metadata
		TargetStructs.push_back(Struct);
		StructIndex = static_cast<uint32_t>(TargetStructs.size() - 1);
	}
}
//---------------------------------------------------------------------
