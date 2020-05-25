#pragma once
#include <Data.h>
#include <vector>
#include <set>
#include <map>

class CThreadSafeLog;

enum class EShaderConstType
{
	Float,
	Int,
	Bool,
	Struct
};

// Don't change values, thay will be saved to file
enum EMaterialParamType : uint8_t
{
	Constant = 0,
	Resource = 1,
	Sampler  = 2
};

struct CMaterialConst
{
	EShaderConstType Type;
	uint32_t SizeInBytes;
};

struct CMaterialParams
{
	std::map<std::string, CMaterialConst> Consts;
	std::set<std::string> Resources;
	std::set<std::string> Samplers;

	bool HasConstant(const std::string& ID) const { return Consts.find(ID) != Consts.cend(); }
	bool HasResource(const std::string& ID) const { return Resources.find(ID) != Resources.cend(); }
	bool HasSampler(const std::string& ID) const { return Samplers.find(ID) != Samplers.cend(); }
};

bool GetEffectMaterialParams(CMaterialParams& Out, const std::string& EffectPath, CThreadSafeLog& Log);
bool WriteMaterialParams(std::ostream& Stream, const CMaterialParams& Table, const Data::CParams& Values, CThreadSafeLog& Log);
bool SaveMaterial(std::ostream& Stream, const std::string& EffectID, const CMaterialParams& Table, const Data::CParams& Values, CThreadSafeLog& Log);

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
		TargetStructs.push_back(std::move(Struct));
		StructIndex = static_cast<uint32_t>(TargetStructs.size() - 1);
	}
}
//---------------------------------------------------------------------

inline bool CheckRegisterOverlapping(uint32_t RegisterStart, uint32_t RegisterCount, const std::set<uint32_t>& Used)
{
	// Fail if overlapping detected. Overlapping data can't be correctly set from effects.
	for (uint32_t r = RegisterStart; r < RegisterStart + RegisterCount; ++r)
		if (Used.find(r) != Used.cend())
			return false;

	return true;
}
//---------------------------------------------------------------------
