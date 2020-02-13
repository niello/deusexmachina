#pragma once
#include <Data/Metadata.h>
#include <vector>
#include <unordered_map>

// Serialization of arbitrary data to binary format.
// TOutput must define <<, TInput must define >>.

// TODO: use additional knowledge about field ranges

namespace DEM
{

template<bool WriteMemberName = false, bool WriteMemberCode = false>
struct BinaryFormatT
{
	template<typename TOutput, typename TValue>
	static inline void Serialize(TOutput& Output, const TValue& Value)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			static_assert(DEM::Meta::CMetadata<TValue>::GetMemberCount() < 256);
			Output << static_cast<uint8_t>(DEM::Meta::CMetadata<TValue>::GetMemberCount());
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Output, &Value](const auto& Member)
			{
				if constexpr (WriteMemberName) Output << Member.GetName();
				if constexpr (WriteMemberCode) Output << Member.GetCode();
				Serialize(Output, Member.GetConstValue(Value));
			});
		}
		else Output << Value;
	}
	//---------------------------------------------------------------------

	//???TODO: extend to all iterable?
	template<typename TOutput, typename TValue>
	static inline void Serialize(TOutput& Output, const std::vector<TValue>& Vector)
	{
		Output << static_cast<uint32_t>(Vector.size());
		for (const auto& Value : Vector)
			Serialize(Output, Value);
	}
	//---------------------------------------------------------------------

	//???TODO: extend to all pair-iterable?
	template<typename TOutput, typename TKey, typename TValue>
	static inline void Serialize(TOutput& Output, const std::unordered_map<TKey, TValue>& Map)
	{
		Output << static_cast<uint32_t>(Map.size());
		for (const auto& [Key, Value] : Map)
		{
			Serialize(Output, Key);
			Serialize(Output, Value);
		}
	}
	//---------------------------------------------------------------------

	template<typename TInput, typename TValue>
	static inline void Deserialize(TInput& Input, TValue& Value)
	{
	}
	//---------------------------------------------------------------------

	template<typename TInput, typename TValue>
	static inline void Deserialize(TInput& Input, std::vector<TValue>& Vector)
	{
	}
	//---------------------------------------------------------------------

	template<typename TInput, typename TKey, typename TValue>
	static inline void Deserialize(TInput& Input, std::unordered_map<TKey, TValue>& Map)
	{
	}
	//---------------------------------------------------------------------
};

using BinaryFormat = BinaryFormatT<false, false>;
using BinaryFormatWithName = BinaryFormatT<true, false>;
using BinaryFormatWithCode = BinaryFormatT<false, true>;

}
