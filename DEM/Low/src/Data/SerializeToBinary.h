#pragma once
#include <Data/Metadata.h>
#include <vector>
#include <unordered_map>

// Serialization of arbitrary data to binary format.
// TOutput must define <<, TInput must define >>.

// TODO: use additional knowledge about field ranges

namespace DEM
{

struct BinaryFormat
{
	template<typename TOutput, typename TValue>
	static inline void Serialize(TOutput& Output, const TValue& Value)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Output, &Value](const auto& Member)
			{
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
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Input, &Value](const auto& Member)
			{
				DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
				Deserialize(Input, FieldValue);
				Member.SetValue(Value, FieldValue);
			});
		}
		else Input >> Value;
	}
	//---------------------------------------------------------------------

	template<typename TInput, typename TValue>
	static inline void Deserialize(TInput& Input, std::vector<TValue>& Vector)
	{
		uint32_t Count;
		Input >> Count;
		Vector.resize(Count);
		for (size_t i = 0; i < Count; ++i)
			Deserialize(Input, Value[i]);
	}
	//---------------------------------------------------------------------

	template<typename TInput, typename TKey, typename TValue>
	static inline void Deserialize(TInput& Input, std::unordered_map<TKey, TValue>& Map)
	{
		uint32_t Count;
		Input >> Count;
		for (size_t i = 0; i < Count; ++i)
		{
			TKey Key;
			TValue Value;
			Deserialize(Input, Key);
			Deserialize(Input, Value);
			Map.emplace(std::move(Key), std::move(Value));
		}
	}
	//---------------------------------------------------------------------
};

}
