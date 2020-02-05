#pragma once
#include <Data/Metadata.h>
#include <vector>
#include <unordered_map>

// Serialization of arbitrary data to compact binary format.
// Uses additional knowledge about field ranges, but always preserves information.
// TOutput must define <<, TInput must define >>.

namespace DEM
{

struct CompactBinaryFormat
{
	template<typename TOutput, typename TKey, typename TValue>
	static inline void SerializeKeyValue(TOutput& Output, TKey Key, const TValue& Value)
	{
	}
	//---------------------------------------------------------------------

	template<typename TOutput, typename TValue>
	static inline void Serialize(TOutput& Output, const TValue& Value)
	{
	}
	//---------------------------------------------------------------------

	template<typename TOutput, typename TValue>
	static inline void Serialize(TOutput& Output, const std::vector<TValue>& Vector)
	{
	}
	//---------------------------------------------------------------------

	template<typename TOutput, typename TKey, typename TValue>
	static inline void Serialize(TOutput& Output, const std::unordered_map<TKey, TValue>& Map)
	{
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

}
