#pragma once
#include <Data/Metadata.h>
#include <Data/IterableTraits.h>
#include <Data/Params.h>
#include <Data/DataArray.h>

// Serialization of arbitrary data to DEM CData

namespace DEM
{

template<typename T>
constexpr bool is_string_compatible_v =
	std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string> ||
	std::is_convertible_v<T, const char*>;

struct ParamsFormat
{
	template<typename TKey, typename TValue>
	static inline void SerializeKeyValue(Data::CParams& Output, TKey Key, const TValue& Value)
	{
		Data::CData ValueData;
		Serialize(ValueData, Value);

		if constexpr (!is_string_compatible_v<TKey>)
			static_assert(false, "CData serialization supports only string map keys");
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
			Output.Set(Key, std::move(ValueData));
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, std::string>)
			Output.Set(CStrID(Key.c_str()), std::move(ValueData));
		else
			Output.Set(CStrID(Key), std::move(ValueData));
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_iterable_v<T>>* = nullptr>
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<T>::GetMemberCount())));
			DEM::Meta::CMetadata<T>::ForEachMember([&Out, &Value](const auto& Member)
			{
				if (!Member.CanSerialize()) return;
				SerializeKeyValue(*Out, Member.GetName(), Member.GetConstValue(Value));
			});
			Output = std::move(Out);
		}
		else if constexpr (Data::CTypeID<T>::IsDeclared)
			Output = Value;
		else
			static_assert(false, "CData serialization supports only types registered with CTypeID");
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_iterable_v<T>>* = nullptr>
	static inline void Serialize(Data::CData& Output, const T& Vector)
	{
		Data::PDataArray Out(n_new(Data::CDataArray(Vector.size())));
		for (const auto& Value : Vector)
		{
			Data::CData ValueData;
			Serialize(ValueData, Value);
			Out->Add(std::move(ValueData));
		}
		Output = std::move(Out);
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void Serialize(Data::CData& Output, const T& Map)
	{
		Data::PParams Out(n_new(Data::CParams(Map.size())));
		for (const auto& [Key, Value] : Map)
			SerializeKeyValue(*Out, Key, Value);
		Output = std::move(Out);
	}
	//---------------------------------------------------------------------

	template<typename TKey, typename TValue>
	static inline bool SerializeKeyValueDiff(Data::CParams& Output, TKey Key, const TValue& Value, const TValue& BaseValue)
	{
		Data::CData ValueData;
		if (!SerializeDiff(ValueData, Value, BaseValue)) return false;

		if constexpr (!is_string_compatible_v<TKey>)
			static_assert(false, "CData serialization supports only string map keys");
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
			Output.Set(Key, std::move(ValueData));
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, std::string>)
			Output.Set(CStrID(Key.c_str()), std::move(ValueData));
		else
			Output.Set(CStrID(Key), std::move(ValueData));

		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_iterable_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Value, const T& BaseValue)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<T>::GetMemberCount())));
			DEM::Meta::CMetadata<T>::ForEachMember([&Out, &Value, &BaseValue](const auto& Member)
			{
				if (!Member.CanSerialize()) return;
				SerializeKeyValueDiff(*Out, Member.GetName(), Member.GetConstValue(Value), Member.GetConstValue(BaseValue));
			});
			if (Out->GetCount())
			{
				Output = std::move(Out);
				return true;
			}
		}
		else if constexpr (Data::CTypeID<T>::IsDeclared)
		{
			if (Value != BaseValue)
			{
				Output = Value;
				return true;
			}
		}
		else
			static_assert(false, "CData serialization supports only types registered with CTypeID");

		return false;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_vector_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Vector, const T& BaseVector)
	{
		// Vector diff is a vector with length of the new value and nulls for equal elements.
		Data::PDataArray Out(n_new(Data::CDataArray(Vector.size())));

		// Save new value of element or null if element didn't change
		bool HasChanges = (Vector.size() != BaseVector.size());
		const size_t MinSize = std::min(Vector.size(), BaseVector.size());
		for (size_t i = 0; i < MinSize; ++i)
		{
			Data::CData Elm;
			HasChanges |= SerializeDiff(Elm, Vector[i], BaseVector[i]);
			Out->Add(Elm);
		}

		if (!HasChanges) return false;

		// Process added elements. If new array is shorter, deleted elements will be detected from diff length.
		if (Vector.size() > MinSize)
		{
			for (size_t i = MinSize; i < Vector.size(); ++i)
			{
				Data::CData Elm;
				Serialize(Elm, Vector[i]);
				Out->Add(Elm);
			}
		}

		Output = Out;
		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_set_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Set, const T& BaseSet)
	{
		// Set diff is two lists - added and deleted elements.
		std::vector<T::value_type> Added;
		std::set_difference(Set.cbegin(), Set.cend(), BaseSet.cbegin(), BaseSet.cend(), std::back_inserter(Added));
		std::vector<T::value_type> Deleted;
		std::set_difference(BaseSet.cbegin(), BaseSet.cend(), Set.cbegin(), Set.cend(), std::back_inserter(Deleted));

		if (Added.empty() && Deleted.empty()) return false;

		Data::PParams Out(n_new(Data::CParams((Added.empty() ? 0 : 1) + (Deleted.empty() ? 0 : 1))));

		if (!Added.empty())
		{
			Data::CData OutAdded;
			Serialize(OutAdded, Added);
			Out->Set(CStrID("Added"), std::move(OutAdded));
		}

		if (!Deleted.empty())
		{
			Data::CData OutDeleted;
			Serialize(OutDeleted, Deleted);
			Out->Set(CStrID("Deleted"), std::move(OutDeleted));
		}

		Output = Out;
		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_unordered_set_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Set, const T& BaseSet)
	{
		// Set diff is two lists - added and deleted elements.
		std::vector<T::value_type> Added;
		std::copy_if(Set.cbegin(), Set.cend(), std::back_inserter(Added), [&BaseSet](const auto& Value)
		{
			return BaseSet.find(Value) == BaseSet.cend();
		});
		std::vector<T::value_type> Deleted;
		std::copy_if(BaseSet.cbegin(), BaseSet.cend(), std::back_inserter(Deleted), [&Set](const auto& Value)
		{
			return Set.find(Value) == Set.cend();
		});

		if (Added.empty() && Deleted.empty()) return false;

		Data::PParams Out(n_new(Data::CParams((Added.empty() ? 0 : 1) + (Deleted.empty() ? 0 : 1))));

		if (!Added.empty())
		{
			Data::CData OutAdded;
			Serialize(OutAdded, Added);
			Out->Set(CStrID("Added"), std::move(OutAdded));
		}

		if (!Deleted.empty())
		{
			Data::CData OutDeleted;
			Serialize(OutDeleted, Deleted);
			Out->Set(CStrID("Deleted"), std::move(OutDeleted));
		}

		Output = Out;
		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Map, const T& BaseMap)
	{
		Data::PParams Out(n_new(Data::CParams(Map.size())));

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
				if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<typename T::key_type>>, CStrID>)
					Out->Set(Key, Data::CData());
				else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<typename T::key_type>>, std::string>)
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

		return false;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_iterable_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			if (auto pParamsPtr = Input.As<Data::PParams>())
			{
				DEM::Meta::CMetadata<T>::ForEachMember([pParams = pParamsPtr->Get(), &Value](const auto& Member)
				{
					if (!Member.CanSerialize()) return;
					if (auto pParam = pParams->Find(CStrID(Member.GetName())))
					{
						DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
						Deserialize(pParam->GetRawValue(), FieldValue);
						Member.SetValue(Value, std::move(FieldValue));
					}
				});
			}
		}
		else if constexpr (Data::CTypeID<T>::IsDeclared)
			Value = Input.GetValue<T>();
		else
			static_assert(false, "CData deserialization supports only types registered with CTypeID");
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_iterable_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Vector)
	{
		Vector.clear();
		if (auto pArrayPtr = Input.As<Data::PDataArray>())
		{
			auto pArray = pArrayPtr->Get();

			if constexpr (std::is_same_v<std::vector<T::value_type>, T>)
				Vector.resize(pArray->GetCount());

			for (size_t i = 0; i < pArray->GetCount(); ++i)
			{
				const auto& ValueData = pArray->At(i);
				if constexpr (std::is_same_v<std::vector<T::value_type>, T>)
				{
					Deserialize(ValueData, Vector[i]);
				}
				else
				{
					typename T::value_type Value;
					Deserialize(ValueData, Value);
					Vector.insert(std::move(Value));
				}
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Map)
	{
		Map.clear();
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			auto pParams = pParamsPtr->Get();
			for (const auto& Param : *pParams)
			{
				typename T::mapped_type Value;
				Deserialize(Param.GetRawValue(), Value);

				if constexpr (!is_string_compatible_v<T::key_type>)
					static_assert(false, "CData deserialization supports only string map keys");
				else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T::key_type>>, CStrID>)
					Map.emplace(Param.GetName(), std::move(Value));
				else
					Map.emplace(Param.GetName().CStr(), std::move(Value));
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_iterable_v<T>>* = nullptr>
	static inline void DeserializeDiff(const Data::CData& Input, T& Value)
	{
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			if (auto pParamsPtr = Input.As<Data::PParams>())
			{
				DEM::Meta::CMetadata<T>::ForEachMember([pParams = pParamsPtr->Get(), &Value](const auto& Member)
				{
					if (!Member.CanSerialize()) return;
					if (auto pParam = pParams->Find(CStrID(Member.GetName())))
					{
						auto& Field = Member.GetValueRef(Value);
						DeserializeDiff(pParam->GetRawValue(), Field);
					}
				});
			}
		}
		else if constexpr (Data::CTypeID<T>::IsDeclared)
			Value = Input.GetValue<T>();
		else
			static_assert(false, "CData deserialization supports only types registered with CTypeID");
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_vector_v<T>>* = nullptr>
	static inline void DeserializeDiff(const Data::CData& Input, T& Vector)
	{
		if (auto pArrayPtr = Input.As<Data::PDataArray>())
		{
			auto pArray = pArrayPtr->Get();

			// Input array size is the new size, discard tail elements as deleted
			Vector.resize(pArray->GetCount());

			// Only non-null values are changed and must be deserialized
			for (size_t i = 0; i < Vector.size(); ++i)
				if (!pArray->At(i).IsVoid())
					DeserializeDiff(pArray->At(i), Vector[i]);
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_set_v<T> || Meta::is_std_unordered_set_v<T>>* = nullptr>
	static inline void DeserializeDiff(const Data::CData& Input, T& Set)
	{
		auto pParamsPtr = Input.As<Data::PParams>();
		if (!pParamsPtr) return;

		Data::PDataArray Deleted;
		if (pParamsPtr->Get()->TryGet(Deleted, CStrID("Deleted")))
		{
			for (const auto& InItem : *Deleted)
			{
				typename T::value_type Value;
				Deserialize(InItem, Value);
				Set.erase(Value);
			}
		}

		Data::PDataArray Added;
		if (pParamsPtr->Get()->TryGet(Added, CStrID("Added")))
		{
			for (const auto& InItem : *Added)
			{
				typename T::value_type Value;
				Deserialize(InItem, Value);
				Set.insert(std::move(Value));
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void DeserializeDiff(const Data::CData& Input, T& Map)
	{
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			auto pParams = pParamsPtr->Get();
			for (const auto& Param : *pParams)
			{
				if (Param.GetRawValue().IsVoid())
				{
					if constexpr (!is_string_compatible_v<T::key_type>)
						static_assert(false, "CData deserialization supports only string map keys");
					else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T::key_type>>, CStrID>)
						Map.erase(Param.GetName());
					else
						Map.erase(Param.GetName().CStr());
				}
				else
				{
					typename T::mapped_type Value;
					Deserialize(Param.GetRawValue(), Value);

					if constexpr (!is_string_compatible_v<T::key_type>)
						static_assert(false, "CData deserialization supports only string map keys");
					else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T::key_type>>, CStrID>)
						Map.insert_or_assign(Param.GetName(), std::move(Value));
					else
						Map.insert_or_assign(Param.GetName().CStr(), std::move(Value));
				}
			}
		}
	}
	//---------------------------------------------------------------------
};

}
