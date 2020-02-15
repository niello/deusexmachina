#pragma once
#include <Data/Metadata.h>
#include <Data/Params.h>

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

	template<typename TValue>
	static inline void Serialize(Data::CData& Output, const TValue& Value)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			Data::PParams Out(n_new(Data::CParams(DEM::Meta::CMetadata<TValue>::GetMemberCount())));
			DEM::Meta::CMetadata<TValue>::ForEachMember([&Out, &Value](const auto& Member)
			{
				SerializeKeyValue(*Out, Member.GetName(), Member.GetConstValue(Value));
			});
			Output = std::move(Out);
		}
		else if constexpr (Data::CTypeID<TValue>::IsDeclared)
			Output = Value;
		else
			static_assert(false, "CData serialization supports only types registered with CTypeID");
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline void Serialize(Data::CData& Output, const std::vector<TValue>& Vector)
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

	template<typename TKey, typename TValue>
	static inline void Serialize(Data::CData& Output, const std::unordered_map<TKey, TValue>& Map)
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
				// Deleted
				Out->Set(CStrID(Key), Data::CData());
		}

		if (Out->GetCount())
		{
			Output = std::move(Out);
			return true;
		}

		return false;
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline void Deserialize(const Data::CData& Input, TValue& Value)
	{
		if constexpr (DEM::Meta::CMetadata<TValue>::IsRegistered)
		{
			if (auto pParamsPtr = Input.As<Data::PParams>())
			{
				DEM::Meta::CMetadata<TValue>::ForEachMember([pParams = pParamsPtr->Get(), &Value](const auto& Member)
				{
					if (auto pParam = pParams->Find(CStrID(Member.GetName())))
					{
						DEM::Meta::TMemberValue<decltype(Member)> FieldValue;
						Deserialize(pParam->GetRawValue(), FieldValue);
						Member.SetValue(Value, FieldValue);
					}
				});
			}
		}
		else if constexpr (Data::CTypeID<TValue>::IsDeclared)
			Value = Input.GetValue<TValue>();
		else
			static_assert(false, "CData deserialization supports only types registered with CTypeID");
	}
	//---------------------------------------------------------------------

	template<typename TValue>
	static inline void Deserialize(const Data::CData& Input, std::vector<TValue>& Vector)
	{
		Vector.clear();
		if (auto pArrayPtr = Input.As<Data::PDataArray>())
		{
			auto pArray = pArrayPtr->Get();
			for (const auto& ValueData : *pArray)
			{
				TValue Value;
				Deserialize(ValueData, Value);
				Vector.push_back(std::move(Value));
			}
		}
	}
	//---------------------------------------------------------------------

	template<typename TKey, typename TValue>
	static inline void Deserialize(const Data::CData& Input, std::unordered_map<TKey, TValue>& Map)
	{
		Map.clear();
		if (auto pParamsPtr = Input.As<Data::PParams>())
		{
			auto pParams = pParamsPtr->Get();
			for (const auto& Param : *pParams)
			{
				TValue Value;
				Deserialize(Param.GetRawValue(), Value);

				if constexpr (!is_string_compatible_v<TKey>)
					static_assert(false, "CData deserialization supports only string map keys");
				else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<TKey>>, CStrID>)
					Map.emplace(Param.GetName(), std::move(Value));
				else
					Map.emplace(Param.GetName().CStr(), std::move(Value));
			}
		}
	}
	//---------------------------------------------------------------------
};

}
