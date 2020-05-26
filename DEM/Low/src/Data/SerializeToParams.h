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

	template<typename TValue>
	static inline bool SerializeDiff(Data::CData& Output, const TValue& Value, const TValue& BaseValue)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<TValue>::GetMemberCount())));
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Out, &Value, &BaseValue](const auto& Member)
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
		else if constexpr (Data::CTypeID<TValue>::IsDeclared)
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

	template<typename TValue>
	static inline bool SerializeDiff(Data::CData& Output, const std::vector<TValue>& Vector, const std::vector<TValue>& BaseVector)
	{
		//???what if count or order changed? iterate the longer one? or maybe always save only curr vector, without diff?
		static_assert(false, "How to implement array diff? All elements, each one saved as diff? If all skipped, skip array.");
		//Data::PDataArray Out(n_new(Data::CDataArray(Vector.size())));
		//for (const auto& Value : Vector)
		//{
		//	Data::CData ValueData;
		//	Serialize(ValueData, Value);
		//	Out->Add(std::move(ValueData));
		//}
		//Output = std::move(Out);
		return false;
	}
	//---------------------------------------------------------------------

	template<typename TKey, typename TValue>
	static inline bool SerializeDiff(Data::CData& Output, const std::unordered_map<TKey, TValue>& Map, const std::unordered_map<TKey, TValue>& BaseMap)
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
			for (const auto& ValueData : *pArray)
			{
				T::value_type Value;
				Deserialize(ValueData, Value);
				if constexpr (std::is_same_v<std::vector<T::value_type>, T>)
					Vector.push_back(std::move(Value));
				else
					Vector.insert(std::move(Value));
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
};

}
