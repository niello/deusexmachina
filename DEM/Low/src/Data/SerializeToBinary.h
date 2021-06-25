#pragma once
#include <Data/Metadata.h>
#include <Data/CategorizationTraits.h>
#include <IO/BinaryWriter.h>
#include <IO/BinaryReader.h>

// Serialization of arbitrary data to binary format

// TODO: use additional knowledge about field ranges for float quantization, max str len for small char counter etc

namespace DEM
{

struct BinaryFormat
{
	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void Serialize(IO::CBinaryWriter& Output, const T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			DEM::Meta::CMetadata<T>::ForEachMember([&Output, &Value](const auto& Member)
			{
				Serialize(Output, Member.GetConstValue(Value));
			});
		}
		else Output << Value;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_collection_v<T>>* = nullptr>
	static inline void Serialize(IO::CBinaryWriter& Output, const T& Vector)
	{
		Output << static_cast<uint32_t>(Vector.size());
		for (const auto& Value : Vector)
			Serialize(Output, Value);
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void Serialize(IO::CBinaryWriter& Output, const T& Map)
	{
		Output << static_cast<uint32_t>(Map.size());
		for (const auto& [Key, Value] : Map)
		{
			Serialize(Output, Key);
			Serialize(Output, Value);
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const T& Value, const T& BaseValue)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			bool HasDiff = false;
			DEM::Meta::CMetadata<T>::ForEachMember([&Output, &Value, &BaseValue, &HasDiff](const auto& Member)
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

	template<typename T, typename std::enable_if_t<Meta::is_std_vector_v<T>>* = nullptr>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const T& Vector, const T& BaseVector)
	{
		// Only 32 bits are saved
		n_assert_dbg(Vector.size() <= std::numeric_limits<uint32_t>().max() &&
			BaseVector.size() <= std::numeric_limits<uint32_t>().max());

		size_t FirstChange = 0;
		const size_t MinSize = std::min(Vector.size(), BaseVector.size());
		for (; FirstChange < MinSize; ++FirstChange)
			if (Vector[FirstChange] != BaseVector[FirstChange]) break;

		if (FirstChange == MinSize && Vector.size() == BaseVector.size()) return false;

		// Write old array size to ensure for compatibility with base vector passed into diff loading
		Output << static_cast<uint32_t>(BaseVector.size());

		// Write new array size to load deleted or added tail elements
		Output << static_cast<uint32_t>(Vector.size());

		for (size_t i = FirstChange; i < MinSize; ++i)
		{
			// Write the index to identify the element. If values are equal in both objects, this will be reverted.
			// TODO: PROFILE! See writing member code SerializeDiff(in is_not_collection_v), the same situation.
			const auto CurrPos = Output.GetStream().Tell();
			Output << static_cast<uint32_t>(i);

			if (!SerializeDiff(Output, Vector[i], BaseVector[i]))
			{
				// "Unwrite" index
				Output.GetStream().Seek(CurrPos, IO::Seek_Begin);
				Output.GetStream().Truncate();
			}
		}

		// End the list of changed elements (much like a trailing \0).
		Output << std::numeric_limits<uint32_t>().max();

		// Process added elements. If new array is shorter, deleted elements will be detected from diff length.
		for (size_t i = MinSize; i < Vector.size(); ++i)
			Serialize(Output, Vector[i]);

		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_set_v<T> || Meta::is_std_unordered_set_v<T>>* = nullptr>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const T& Set, const T& BaseSet)
	{
		size_t AddedCount = 0, DeletedCount = 0;
		SetDifference(Set, BaseSet, [&AddedCount](const auto&) { ++AddedCount; });
		SetDifference(BaseSet, Set, [&DeletedCount](const auto&) { ++DeletedCount; });

		if (!AddedCount && !DeletedCount) return false;

		// Only 32 bits are saved
		n_assert_dbg(AddedCount <= std::numeric_limits<uint32_t>().max() &&
			DeletedCount <= std::numeric_limits<uint32_t>().max());

		Output << static_cast<uint32_t>(DeletedCount);
		if (DeletedCount)
			SetDifference(BaseSet, Set, [&Output, &DeletedCount](const auto& Value)
			{
				Serialize(Output, Value);
				return --DeletedCount > 0;
			});

		Output << static_cast<uint32_t>(AddedCount);
		if (AddedCount)
			SetDifference(Set, BaseSet, [&Output, &AddedCount](const auto& Value)
			{
				Serialize(Output, Value);
				return --AddedCount > 0;
			});

		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline bool SerializeDiff(IO::CBinaryWriter& Output, const T& Map, const T& BaseMap)
	{
		// Can't choose value for termination key, so can't just do k-v-k-v-...-k(term).
		// Have to calculate how many keys will be in each category in advance.
		size_t AddedCount = 0, ModifiedCount = 0, DeletedCount = 0;
		for (const auto& [Key, Value] : Map)
		{
			auto BaseIt = BaseMap.find(Key);
			if (BaseIt == BaseMap.cend()) ++ AddedCount;
			else if (BaseIt->second != Value) ++ModifiedCount;
		}
		for (const auto& [Key, Value] : BaseMap)
			if (Map.find(Key) == Map.cend()) ++DeletedCount;

		if (!AddedCount && !DeletedCount && !ModifiedCount) return false;

		// Only 32 bits are saved
		n_assert_dbg(AddedCount <= std::numeric_limits<uint32_t>().max() &&
			DeletedCount <= std::numeric_limits<uint32_t>().max() &&
			ModifiedCount <= std::numeric_limits<uint32_t>().max());

		Output << static_cast<uint32_t>(DeletedCount);
		if (DeletedCount)
		{
			for (const auto& [Key, Value] : BaseMap)
			{
				if (Map.find(Key) == Map.cend())
				{
					Serialize(Output, Key);
					if (--DeletedCount == 0) break;
				}
			}
		}

		Output << static_cast<uint32_t>(ModifiedCount);
		if (ModifiedCount)
		{
			for (const auto& [Key, Value] : Map)
			{
				auto BaseIt = BaseMap.find(Key);
				if (BaseIt != BaseMap.cend() && BaseIt->second != Value)
				{
					Serialize(Output, Key);
					SerializeDiff(Output, Value, BaseIt->second);
					if (--ModifiedCount == 0) break;
				}
			}
		}

		Output << static_cast<uint32_t>(AddedCount);
		if (AddedCount)
		{
			for (const auto& [Key, Value] : Map)
			{
				if (BaseMap.find(Key) == BaseMap.cend())
				{
					Serialize(Output, Key);
					Serialize(Output, Value);
					if (--AddedCount == 0) break;
				}
			}
		}

		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void Deserialize(IO::CBinaryReader& Input, T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			DEM::Meta::CMetadata<T>::ForEachMember([&Input, &Value](const auto& Member)
			{
				DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
				Deserialize(Input, FieldValue);
				Member.SetValue(Value, std::move(FieldValue));
			});
		}
		else Input >> Value;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_collection_v<T>>* = nullptr>
	static inline void Deserialize(IO::CBinaryReader& Input, T& Vector)
	{
		uint32_t Count;
		Input >> Count;

		if constexpr (std::is_same_v<std::vector<T::value_type>, T>)
			Vector.resize(Count);
		else
			Vector.clear();

		for (size_t i = 0; i < Count; ++i)
		{
			if constexpr (std::is_same_v<std::vector<T::value_type>, T>)
			{
				Deserialize(Input, Vector[i]);
			}
			else
			{
				typename T::value_type Value;
				Deserialize(Input, Value);
				Vector.insert(std::move(Value));
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void Deserialize(IO::CBinaryReader& Input, T& Map)
	{
		Map.clear();

		uint32_t Count;
		Input >> Count;
		for (size_t i = 0; i < Count; ++i)
		{
			T::key_type Key;
			T::mapped_type Value;
			Deserialize(Input, Key);
			Deserialize(Input, Value);
			Map.emplace(std::move(Key), std::move(Value));
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void DeserializeDiff(IO::CBinaryReader& Input, T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			// TODO: PERF, can iterate members once if saved codes are guaranteed to keep the same order as in members
			auto Code = Input.Read<uint32_t>();
			while (Code != DEM::Meta::NO_MEMBER_CODE)
			{
				DEM::Meta::CMetadata<T>::ForEachMember([&Input, &Value, &Code](const auto& Member)
				{
					if (Member.CanSerialize() && Code == Member.GetCode())
					{
						if constexpr (Member.CanGetWritableRef())
						{
							DeserializeDiff(Input, Member.GetValueRef(Value));
						}
						else
						{
							auto FieldValue = Member.GetConstValue(Value);
							DeserializeDiff(Input, FieldValue);
							Member.SetValue(Value, std::move(FieldValue));
						}
					}
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

	template<typename T, typename std::enable_if_t<Meta::is_std_vector_v<T>>* = nullptr>
	static inline void DeserializeDiff(IO::CBinaryReader& Input, T& Vector)
	{
		const auto OldSize = Input.Read<uint32_t>();
		const auto NewSize = Input.Read<uint32_t>();

		Vector.resize(NewSize);

		uint32_t ChangedIndex = Input.Read<uint32_t>();
		while (ChangedIndex < std::numeric_limits<uint32_t>().max())
		{
			DeserializeDiff(Input, Vector[ChangedIndex]);
			ChangedIndex = Input.Read<uint32_t>();
		}

		for (size_t i = OldSize; i < NewSize; ++i)
			Deserialize(Input, Vector[i]);
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_set_v<T> || Meta::is_std_unordered_set_v<T>>* = nullptr>
	static inline void DeserializeDiff(IO::CBinaryReader& Input, T& Set)
	{
		const auto DeletedSize = Input.Read<uint32_t>();
		for (size_t i = 0; i < DeletedSize; ++i)
		{
			typename T::value_type Value;
			Deserialize(Input, Value);
			Set.erase(Value);
		}

		const auto AddedSize = Input.Read<uint32_t>();
		for (size_t i = 0; i < AddedSize; ++i)
		{
			typename T::value_type Value;
			Deserialize(Input, Value);
			Set.insert(Value);
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void DeserializeDiff(IO::CBinaryReader& Input, T& Map)
	{
		const auto DeletedSize = Input.Read<uint32_t>();
		for (size_t i = 0; i < DeletedSize; ++i)
		{
			typename T::key_type Key;
			Deserialize(Input, Key);
			Map.erase(Key);
		}

		const auto ModifiedSize = Input.Read<uint32_t>();
		for (size_t i = 0; i < ModifiedSize; ++i)
		{
			typename T::key_type Key;
			Deserialize(Input, Key);

			auto It = Map.find(Key);
			if (It != Map.cend())
			{
				DeserializeDiff(Input, It->second);
			}
			else
			{
				// Must not happen normally, but let's handle it
				typename T::mapped_type Value;
				DeserializeDiff(Input, Value);
				Map.emplace(std::move(Key), std::move(Value));
			}
		}

		const auto AddedSize = Input.Read<uint32_t>();
		for (size_t i = 0; i < AddedSize; ++i)
		{
			typename T::key_type Key;
			typename T::mapped_type Value;
			Deserialize(Input, Key);
			Deserialize(Input, Value);
			Map.emplace(std::move(Key), std::move(Value));
		}
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline constexpr size_t GetMaxDiffSize()
	{
		if constexpr (Meta::is_single_iterable_v<TValue> || Meta::is_pair_iterable_v<TValue>)
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
