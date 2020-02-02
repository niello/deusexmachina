#pragma once
#include <Data/Metadata.h>

// Generates (de)serialization for arbitrary user-defined formats and inputs/outputs.
// Output must define operator << for each supported data type. Input - same for >>.
// This requirement can be overridden by specialization.

// FIXME: pass by value for primitive types! use conditional for primitives, result in "T" vs "const T&"?
// TODO: need decay_t to serialize by value and by ref through the same function! or cast to passable into Serialize.
// TODO: some formats may use name, others may use code! need some flag in a format traits to choose behaviour!
// TODO: serialize diff

namespace DEM::Meta
{

template<typename TFormat, typename TOutput, typename TValue, std::enable_if_t<!CMetadata<std::decay_t<TValue>>::IsRegistered>* = nullptr>
inline void Serialize(TOutput& Output, const TValue& Value)
{
	TFormat::SerializeValue(Output, Value);
}

template<typename TFormat, typename TOutput, typename TValue, std::enable_if_t<CMetadata<std::decay_t<TValue>>::IsRegistered>* = nullptr>
inline void Serialize(TOutput& Output, const TValue& Value)
{
	CMetadata<TValue>::ForEachMember([&Output, &Value](const auto& Member)
	{
		TFormat::SerializeMember(Output, Value, Member);
	});
}

struct TextFormat
{
	template<typename TOutput, typename TValue>
	static void SerializeValue(TOutput& Output, const TValue& Value)
	{
		if constexpr (std::is_same_v<std::decay_t<TValue>, std::string> || std::is_convertible_v<TValue, const char*>)
			Output << Value;
		else
			Output << std::to_string(Value);
	}

	template<typename TOutput, typename TKey, typename TValue>
	static void SerializeKeyValue(TOutput& Output, const TKey& Key, const TValue& Value)
	{
		SerializeValue(Output, Key);
		Output << " = ";
		Serialize<TextFormat>(Output, Value);
		Output << "; ";
	}

	template<typename TOutput, typename TValue, typename TMember>
	static void SerializeMember(TOutput& Output, const TValue& Value, const TMember& Member)
	{
		SerializeKeyValue(Output, Member.GetName(), Member.GetConstValue(Value));
	}
};

}
