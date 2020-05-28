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

template<typename T, typename TCallback>
void SetDifference(const std::set<T>& a, const std::set<T>& b, TCallback Callback)
{
	auto ItCurrA = a.cbegin();
	auto ItCurrB = b.cbegin();
	while (ItCurrA != a.cend() && ItCurrB != b.cend())
	{
		if (*ItCurrA < *ItCurrB)
		{
			if constexpr (std::is_invocable_r_v<bool, TCallback, const T&>)
			{
				if (!Callback(*ItCurrA++)) return;
			}
			else if constexpr (std::is_invocable_r_v<void, TCallback, const T&>)
			{
				Callback(*ItCurrA++);
			} 
			else static_assert(false, "Callback must accept const T& and return void or bool");
		}
		else
		{
			if (!(*ItCurrB < *ItCurrA)) ++ItCurrA;
			++ItCurrB;
		}
	}

	while (ItCurrA != a.cend())
	{
		if constexpr (std::is_invocable_r_v<bool, TCallback, const T&>)
		{
			if (!Callback(*ItCurrA++)) return;
		}
		else if constexpr (std::is_invocable_r_v<void, TCallback, const T&>)
		{
			Callback(*ItCurrA++);
		} 
		else static_assert(false, "Callback must accept const T& and return void or bool");
	}
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
