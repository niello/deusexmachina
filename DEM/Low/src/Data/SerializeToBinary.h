#pragma once
#include <Data/Metadata.h>
#include <IO/BinaryWriter.h>
#include <IO/BinaryReader.h>
#include <vector>
#include <unordered_map>

// Serialization of arbitrary data to binary format

// TODO: use additional knowledge about field ranges for float quantization, max str len for small char counter etc

namespace DEM
{

struct BinaryFormat
{
	template<typename TValue>
	static inline void Serialize(IO::CBinaryWriter& Output, const TValue& Value)
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
	template<typename TValue>
	static inline void Serialize(IO::CBinaryWriter& Output, const std::vector<TValue>& Vector)
	{
		Output << static_cast<uint32_t>(Vector.size());
		for (const auto& Value : Vector)
			Serialize(Output, Value);
	}
	//---------------------------------------------------------------------

	//???TODO: extend to all pair-iterable?
	template<typename TKey, typename TValue>
	static inline void Serialize(IO::CBinaryWriter& Output, const std::unordered_map<TKey, TValue>& Map)
	{
		Output << static_cast<uint32_t>(Map.size());
		for (const auto& [Key, Value] : Map)
		{
			Serialize(Output, Key);
			Serialize(Output, Value);
		}
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const TValue& Value, const TValue& BaseValue)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			bool HasDiff = false;
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Output, &Value, &BaseValue, &HasDiff](const auto& Member)
			{
				// Can diff-serialize only members with code, otherwise it couldn't be identified
				// TODO: in addition could have per-member constexpr bool "need serialization/need diff/...?"
				if (Member.GetCode() == DEM::Meta::NO_MEMBER_CODE) return;

				// Write the code to identify the member. If member is equal in both objects, this will be reverted.
				// TODO: PROFILE! Code is written for each member, but changed % is typically much less than 100.
				// If too slow, can write to RAM buffer and explicitly deny to flush. Then with first difference
				// detected flush the code too. Discarding code in RAM is just a pointer subtraction, not fseek.
				const auto CurrPos = Output.GetStream().Tell();
				Output << Member.GetCode();

				if (SerializeDiff(Output, Member.GetConstValue(Value), Member.GetConstValue(BaseValue)))
					HasDiff = true;
				else
				{
					// "Unwrite" code
					Output.GetStream().Seek(CurrPos, IO::Seek_Begin);
					Output.GetStream().Truncate();
				}
			});

			// End the list of changed members (much like a trailing \0).
			// If HasDiff is false, calling code must skip the object entirely instead.
			if (HasDiff) Output << DEM::Meta::NO_MEMBER_CODE;

			return HasDiff;
		}
		else if (BaseValue != Value)
		{
			Output << Value;
			return true;
		}
		return false;
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline void Deserialize(IO::CBinaryReader& Input, TValue& Value)
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

	template<typename TValue>
	static inline void Deserialize(IO::CBinaryReader& Input, std::vector<TValue>& Vector)
	{
		uint32_t Count;
		Input >> Count;
		Vector.resize(Count);
		for (size_t i = 0; i < Count; ++i)
			Deserialize(Input, Vector[i]);
	}
	//---------------------------------------------------------------------

	template<typename TKey, typename TValue>
	static inline void Deserialize(IO::CBinaryReader& Input, std::unordered_map<TKey, TValue>& Map)
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

	template<typename TValue>
	static inline void DeserializeDiff(IO::CBinaryReader& Input, TValue& Value, const TValue& BaseValue)
	{
		NOT_IMPLEMENTED;
	}
	//---------------------------------------------------------------------

	template<typename>
	struct is_std_vector : std::false_type {};

	template<typename T, typename V>
	struct is_std_vector<std::vector<T, V>> : std::true_type {};

	template<typename>
	struct is_std_map : std::false_type {};

	template<typename T, typename K, typename V>
	struct is_std_map<std::unordered_map<T, K, V>> : std::true_type {};

	template<typename TValue>
	static inline constexpr size_t GetMaxDiffSize()
	{
		if constexpr (is_std_vector<TValue>::value)
		{
			return std::numeric_limits<size_t>().max();
		}
		else if constexpr (is_std_map<TValue>::value)
		{
			return std::numeric_limits<size_t>().max();
		}
		else if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			size_t Size = 0;
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Size](const auto& Member)
			{
				if (Member.GetCode() == DEM::Meta::NO_MEMBER_CODE) return;

				if (Size < std::numeric_limits<size_t>().max())
				{
					constexpr auto MemberSize = GetMaxDiffSize<DEM::Meta::TMemberValue<decltype(Member)>>();
					if constexpr (MemberSize == std::numeric_limits<size_t>().max())
						Size = MemberSize;
					else
						Size += sizeof(Member.GetCode()) + MemberSize;
				}
			});
			return Size;
		}
		else return sizeof(TValue);
	}
	//---------------------------------------------------------------------
};

}
