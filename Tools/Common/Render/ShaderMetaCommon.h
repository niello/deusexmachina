#pragma once
#include <vector>
#include <set>
#include <map>

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
