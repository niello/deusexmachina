#pragma once
#include <Data/Metadata.h>
#include <IO/BinaryWriter.h>
#include <IO/BinaryReader.h>

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
				if (!Member.CanSerialize()) return;

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
			Output << DEM::Meta::NO_MEMBER_CODE;

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
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const std::vector<TValue>& Vector, const std::vector<TValue>& BaseVector)
	{
		static_assert(false, "Array diff - as map, but with int keys?");
		return false;
	}
	//---------------------------------------------------------------------

	template<typename TKey, typename TValue>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const std::unordered_map<TKey, TValue>& Map, const std::unordered_map<TKey, TValue>& BaseMap)
	{
		/*
		for (const auto& [Key, Value] : Map)
		{
			auto BaseIt = BaseMap.find(Key);
			if (BaseIt == BaseMap.cend())
				// Added
				SerializeKeyValue(*Out, Key, Value);
			else
				// Modified
				SerializeKeyValueDiff(*Out, Key, Value, BaseIt->second);
		}

		for (const auto& [Key, Value] : BaseMap)
		{
			if (Map.find(Key) == Map.cend())
			{
				// Deleted
				if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
					Out->Set(Key, Data::CData());
				else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, std::string>)
					Out->Set(CStrID(Key.c_str()), Data::CData());
				else
					Out->Set(CStrID(Key), Data::CData());
			}
		}

		if (Out->GetCount())
		{
			Output = std::move(Out);
			return true;
		}
		*/

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
				Member.SetValue(Value, std::move(FieldValue));
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
	static inline void DeserializeDiff(IO::CBinaryReader& Input, TValue& Value)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			// TODO: PERF, can iterate members once if saved codes are guaranteed to keep the same order as in members
			auto Code = Input.Read<uint32_t>();
			while (Code != DEM::Meta::NO_MEMBER_CODE)
			{
				DEM::Meta::CMetadata<TValue>::ForEachMember([&Input, &Value, &Code](const auto& Member)
				{
					if (!Member.CanSerialize() || Code != Member.GetCode()) return;
					DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
					DeserializeDiff(Input, FieldValue);
					Member.SetValue(Value, std::move(FieldValue));
				});

				Code = Input.Read<uint32_t>();
			}
		}
		else
		{
			// If we're here, there is diff data to load
			Input >> Value;
		}
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
				if (!Member.CanSerialize()) return;

				if (Size < std::numeric_limits<size_t>().max())
				{
					constexpr auto MemberSize = GetMaxDiffSize<DEM::Meta::TMemberValue<decltype(Member)>>();
					if constexpr (MemberSize == std::numeric_limits<size_t>().max())
						Size = std::numeric_limits<size_t>().max();
					else
						Size += sizeof(Member.GetCode()) + MemberSize;
				}
			});

			// Terminating code
			if (Size <= (std::numeric_limits<size_t>().max() - sizeof(uint32_t)))
				Size += sizeof(uint32_t);
			else
				Size = std::numeric_limits<size_t>().max();

			return Size;
		}
		else return sizeof(TValue);
	}
	//---------------------------------------------------------------------
};

}
