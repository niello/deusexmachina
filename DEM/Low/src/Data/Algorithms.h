#pragma once
//#include <Data/FunctionTraits.h>
#include <algorithm>
#include <set>
#include <unordered_set>

// Utility algorithms and wrappers over std algorithms

namespace DEM
{

//template<typename T>
//auto Erase(std::vector<T>& Vector, typename std::vector<T>::iterator It)
//{
//}
////---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' that do not appear in 'b'
template<typename TCollection, typename TCallback>
void SortedDifference(const TCollection& a, const TCollection& b, TCallback Callback)
{
	auto ItCurrA = a.cbegin();
	auto ItCurrB = b.cbegin();
	while (ItCurrA != a.cend() && ItCurrB != b.cend())
	{
		if (*ItCurrA < *ItCurrB)
		{
			if constexpr (std::is_invocable_r_v<bool, TCallback, typename TCollection::const_iterator>)
			{
				if (!Callback(ItCurrA++)) return;
			}
			else if constexpr (std::is_invocable_r_v<void, TCallback, typename TCollection::const_iterator>)
			{
				Callback(ItCurrA++);
			}
			else static_assert(false, "Callback must accept const_iterator and return void or bool");
		}
		else
		{
			if (!(*ItCurrB < *ItCurrA)) ++ItCurrA;
			++ItCurrB;
		}
	}

	while (ItCurrA != a.cend())
	{
		if constexpr (std::is_invocable_r_v<bool, TCallback, typename TCollection::const_iterator>)
		{
			if (!Callback(ItCurrA++)) return;
		}
		else if constexpr (std::is_invocable_r_v<void, TCallback, typename TCollection::const_iterator>)
		{
			Callback(ItCurrA++);
		}
		else static_assert(false, "Callback must accept const_iterator and return void or bool");
	}
}
//---------------------------------------------------------------------

template<typename T, typename TCallback>
void SetDifference(const std::set<T>& a, const std::set<T>& b, TCallback Callback)
{
	SortedDifference(a, b, std::forward(Callback));
}
//---------------------------------------------------------------------

template<typename T, typename TCallback>
void SetDifference(const std::unordered_set<T>& a, const std::unordered_set<T>& b, TCallback Callback)
{
	for (const auto& Value : a)
		if (b.find(Value) == b.cend())
			Callback(Value);
}
//---------------------------------------------------------------------

}
