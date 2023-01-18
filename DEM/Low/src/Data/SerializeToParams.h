#pragma once
#include <Data/Metadata.h>
#include <Data/CategorizationTraits.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/Algorithms.h>

// Serialization of arbitrary data to DEM CData

namespace DEM
{

template<typename T>
constexpr bool is_string_compatible_v =
	std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, CStrID> ||
	std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string> ||
	std::is_convertible_v<T, const char*>;

namespace Serialization
{

template<typename T, typename Enable = void>
struct ParamsFormat
{
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		static_assert(std::is_same_v<T, void>, "DEM::Serialization::ParamsFormat > no serialization specified for type!");
	}

	static inline void Deserialize(const Data::CData& Input, T& Value)
	{
		static_assert(std::is_same_v<T, void>, "DEM::Serialization::ParamsFormat > no deserialization specified for type!");
	}

	static inline bool SerializeDiff(Data::CData& Output, const T& Value, const T& BaseValue)
	{
		static_assert(std::is_same_v<T, void>, "DEM::Serialization::ParamsFormat > no diff serialization specified for type!");
	}

	static inline void DeserializeDiff(const Data::CData& Input, T& Value)
	{
		static_assert(std::is_same_v<T, void>, "DEM::Serialization::ParamsFormat > no diff deserialization specified for type!");
	}
};

}

struct ParamsFormat
{
	template<typename TKey, typename TValue>
	static inline void SerializeKeyValue(Data::CParams& Output, TKey Key, const TValue& Value)
	{
		static_assert(is_string_compatible_v<TKey>, "CData serialization supports only string map keys");

		Data::CData ValueData;
		Serialize(ValueData, Value);

		if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
			Output.Set(Key, std::move(ValueData));
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, std::string>)
			Output.Set(CStrID(Key.c_str()), std::move(ValueData));
		else
			Output.Set(CStrID(Key), std::move(ValueData));
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		Serialization::ParamsFormat<T>::Serialize(Output, Value);
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_collection_v<T>>* = nullptr>
	static inline void Serialize(Data::CData& Output, const T& Vector)
	{
		Data::PDataArray Out;
		if constexpr (std::is_array_v<T>)
			Out = n_new(Data::CDataArray(std::extent_v<T>));
		else
			Out = n_new(Data::CDataArray(Vector.size()));
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
		static_assert(is_string_compatible_v<TKey>, "CData serialization supports only string map keys");

		Data::CData ValueData;
		if (!SerializeDiff(ValueData, Value, BaseValue)) return false;

		if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
			Output.Set(Key, std::move(ValueData));
		else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, std::string>)
			Output.Set(CStrID(Key.c_str()), std::move(ValueData));
		else
			Output.Set(CStrID(Key), std::move(ValueData));

		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Value, const T& BaseValue)
	{
		// FIXME: TMP HACK! Need to call Serialize<T> with != check for SerializeDiff of each T without metadata and explicit specialization!
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			return Serialization::ParamsFormat<T>::SerializeDiff(Output, Value, BaseValue);
		}
		else if (Value != BaseValue)
		{
			Serialization::ParamsFormat<T>::Serialize(Output, Value);
			return true;
		}

		return false;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<std::is_array_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Vector, const T& BaseVector)
	{
		constexpr auto ArraySize = std::extent_v<T>;

		// Vector diff is a vector with length of the new value and nulls for equal elements.
		Data::PDataArray Out(n_new(Data::CDataArray(ArraySize)));

		// Save new value of element or null if element didn't change
		bool HasChanges = false;
		for (size_t i = 0; i < ArraySize; ++i)
		{
			Data::CData Elm;
			HasChanges |= SerializeDiff(Elm, Vector[i], BaseVector[i]);
			Out->Add(std::move(Elm));
		}

		Output = std::move(Out);
		return HasChanges;
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
			Out->Add(std::move(Elm));
		}

		if (!HasChanges) return false;

		// Process added elements. If new array is shorter, deleted elements will be detected from diff length.
		for (size_t i = MinSize; i < Vector.size(); ++i)
		{
			Data::CData Elm;
			Serialize(Elm, Vector[i]);
			Out->Add(std::move(Elm));
		}

		Output = std::move(Out);
		return true;
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_std_set_v<T> || Meta::is_std_unordered_set_v<T>>* = nullptr>
	static inline bool SerializeDiff(Data::CData& Output, const T& Set, const T& BaseSet)
	{
		Data::CData Added;
		Data::CData Deleted;
		SetDifference(Set, BaseSet, [&Added](const auto& Value)
		{
			if (Added.IsVoid()) Added = Data::PDataArray(n_new(Data::CDataArray()));
			Data::CData ValueData;
			Serialize(ValueData, Value);
			Added.As<Data::PDataArray>()->Get()->Add(std::move(ValueData));
		});
		SetDifference(BaseSet, Set, [&Deleted](const auto& Value)
		{
			if (Deleted.IsVoid()) Deleted = Data::PDataArray(n_new(Data::CDataArray()));
			Data::CData ValueData;
			Serialize(ValueData, Value);
			Deleted.As<Data::PDataArray>()->Get()->Add(std::move(ValueData));
		});

		if (Added.IsVoid() && Deleted.IsVoid()) return false;

		Data::PParams Out(n_new(Data::CParams((Added.IsVoid() ? 0 : 1) + (Deleted.IsVoid() ? 0 : 1))));

		if (!Added.IsVoid()) Out->Set(CStrID("Added"), std::move(Added));
		if (!Deleted.IsVoid()) Out->Set(CStrID("Deleted"), std::move(Deleted));

		Output = std::move(Out);
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

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Value)
	{
		Serialization::ParamsFormat<T>::Deserialize(Input, Value);
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<std::is_array_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Vector)
	{
		if (auto pArrayPtr = Input.As<Data::PDataArray>())
		{
			auto pArray = pArrayPtr->Get();
			const auto MinSize = std::min(pArray->GetCount(), std::extent_v<T>);
			for (size_t i = 0; i < MinSize; ++i)
				Deserialize(pArray->At(i), Vector[i]);
		}
		else
		{
			// Try to deserialize the value to the first element of the array
			Deserialize(Input, Vector[0]);
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_single_collection_v<T> && !std::is_array_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Vector)
	{
		Vector.clear();
		if (auto pArrayPtr = Input.As<Data::PDataArray>())
		{
			auto pArray = pArrayPtr->Get();

			if constexpr (std::is_same_v<std::vector<T::value_type>, T>) // Check that the collection is std vector
				Vector.resize(pArray->GetCount());

			for (size_t i = 0; i < pArray->GetCount(); ++i)
			{
				const auto& ValueData = pArray->At(i);
				if constexpr (std::is_same_v<std::vector<T::value_type>, T>) // Check that the collection is std vector
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
		else
		{
			// Try to deserialize the value as a vector of a single element
			Vector.resize(1);
			Deserialize(Input, Vector[0]);
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_pair_iterable_v<T>>* = nullptr>
	static inline void Deserialize(const Data::CData& Input, T& Map)
	{
		static_assert(is_string_compatible_v<T::key_type>, "CData deserialization supports only string map keys");

		constexpr bool IsStrIDKey = std::is_same_v<std::remove_cv_t<std::remove_reference_t<T::key_type>>, CStrID>;

		Map.clear();
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			auto pParams = pParamsPtr->Get();
			for (const auto& Param : *pParams)
			{
				typename T::mapped_type* pValue;
				if constexpr (IsStrIDKey)
					pValue = &Map[Param.GetName()];
				else
					pValue = &Map[Param.GetName().CStr()];

				Deserialize(Param.GetRawValue(), *pValue);
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename T, typename std::enable_if_t<Meta::is_not_collection_v<T>>* = nullptr>
	static inline void DeserializeDiff(const Data::CData& Input, T& Value)
	{
		// FIXME: TMP HACK! Need to call Deserialize<T> for DeserializeDiff of each T without metadata and explicit specialization!
		if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
			Serialization::ParamsFormat<T>::DeserializeDiff(Input, Value);
		else
			Serialization::ParamsFormat<T>::Deserialize(Input, Value);
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
		static_assert(is_string_compatible_v<T::key_type>, "CData deserialization supports only string map keys");

		constexpr bool IsStrIDKey = std::is_same_v<std::remove_cv_t<std::remove_reference_t<T::key_type>>, CStrID>;

		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			auto pParams = pParamsPtr->Get();
			for (const auto& Param : *pParams)
			{
				if (Param.GetRawValue().IsVoid())
				{
					if constexpr (IsStrIDKey)
						Map.erase(Param.GetName());
					else
						Map.erase(Param.GetName().CStr());
				}
				else
				{
					typename T::mapped_type* pValue;
					if constexpr (IsStrIDKey)
						pValue = &Map[Param.GetName()];
					else
						pValue = &Map[Param.GetName().CStr()];

					Deserialize(Param.GetRawValue(), *pValue);
				}
			}
		}
	}
	//---------------------------------------------------------------------
};

namespace Serialization
{

template<typename T>
struct ParamsFormat<T, typename std::enable_if_t<Data::CTypeID<T>::IsDeclared>>
{
	static inline void Serialize(Data::CData& Output, const T& Value) { Output = Value; }
	static inline void Deserialize(const Data::CData& Input, T& Value) { Value = Input.GetValue<T>(); }
};

template<typename T>
struct ParamsFormat<T, typename std::enable_if_t<std::is_integral_v<T> && !Data::CTypeID<T>::IsDeclared>>
{
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		const int IntValue = Value; // To issue compiler warning if data loss is possible
		Output = IntValue;
	}

	static inline void Deserialize(const Data::CData& Input, T& Value) { Value = Input.GetValue<int>(); }
};

template<typename T>
struct ParamsFormat<T, typename std::enable_if_t<std::is_floating_point_v<T> && !Data::CTypeID<T>::IsDeclared>>
{
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		const float FloatValue = Value; // To issue compiler warning if data loss is possible
		Output = FloatValue;
	}

	static inline void Deserialize(const Data::CData& Input, T& Value) { Value = Input.GetValue<float>(); }
};

//!!!FIXME: TMP, use std::string instead of CString!
template<typename... TTraits>
struct ParamsFormat<std::basic_string<TTraits...>>
{
	static inline void Serialize(Data::CData& Output, const std::basic_string<TTraits...>& Value) { Output = CString(Value.c_str()); }
	static inline void Deserialize(const Data::CData& Input, std::basic_string<TTraits...>& Value) { Value = Input.GetValue<CString>().CStr(); }
};

template<typename T>
struct ParamsFormat<T, typename std::enable_if_t<Meta::is_not_collection_v<T> && Meta::CMetadata<T>::IsRegistered>>
{
	static inline void Serialize(Data::CData& Output, const T& Value)
	{
		Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<T>::GetMemberCount())));
		DEM::Meta::CMetadata<T>::ForEachMember([&Out, &Value](const auto& Member)
		{
			if (Member.CanSerialize())
				DEM::ParamsFormat::SerializeKeyValue(*Out, Member.GetName(), Member.GetConstValue(Value));
		});
		Output = std::move(Out);
	}

	static inline void Deserialize(const Data::CData& Input, T& Value)
	{
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			DEM::Meta::CMetadata<T>::ForEachMember([pParams = pParamsPtr->Get(), &Value](const auto& Member)
			{
				if (!Member.CanSerialize()) return;
				if (auto pParam = pParams->Find(CStrID(Member.GetName())))
				{
					DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
					DEM::ParamsFormat::Deserialize(pParam->GetRawValue(), FieldValue);
					Member.SetValue(Value, std::move(FieldValue));
				}
			});
		}
	}

	static inline bool SerializeDiff(Data::CData& Output, const T& Value, const T& BaseValue)
	{
		Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<T>::GetMemberCount())));
		DEM::Meta::CMetadata<T>::ForEachMember([&Out, &Value, &BaseValue](const auto& Member)
		{
			if (Member.CanSerialize())
				DEM::ParamsFormat::SerializeKeyValueDiff(*Out, Member.GetName(), Member.GetConstValue(Value), Member.GetConstValue(BaseValue));
		});
		if (Out->GetCount())
		{
			Output = std::move(Out);
			return true;
		}
		return false;
	}

	static inline void DeserializeDiff(const Data::CData& Input, T& Value)
	{
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			DEM::Meta::CMetadata<T>::ForEachMember([pParams = pParamsPtr->Get(), &Value](const auto& Member)
			{
				if (!Member.CanSerialize()) return;
				if (auto pParam = pParams->Find(CStrID(Member.GetName())))
				{
					if constexpr (Member.CanGetWritableRef())
					{
						DEM::ParamsFormat::DeserializeDiff(pParam->GetRawValue(), Member.GetValueRef(Value));
					}
					else
					{
						auto FieldValue = Member.GetConstValue(Value);
						DEM::ParamsFormat::DeserializeDiff(pParam->GetRawValue(), FieldValue);
						Member.SetValue(Value, std::move(FieldValue));
					}
				}
			});
		}
	}
};

}

}
