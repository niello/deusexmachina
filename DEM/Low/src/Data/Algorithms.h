#pragma once
//#include <Data/FunctionTraits.h>
#include <algorithm>
#include <set>
#include <unordered_set>

// Utility algorithms and wrappers over std algorithms

namespace DEM::Algo
{

//template<typename T>
//auto Erase(std::vector<T>& Vector, typename std::vector<T>::iterator It)
//{
//}
////---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' that do not appear in 'b'
template<typename TCollection, typename TCallback>
inline void SortedDifference(const TCollection& a, const TCollection& b, TCallback Callback)
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

// Calls a Callback with iterators to elements from 'a' and 'b'. When elements with the same key
// exist in both sets, the callback is invoked for them once with both elements valid.
template<typename TCollectionA, typename TCollectionB, typename TLess, typename TCallback>
DEM_FORCE_INLINE void SortedUnion(TCollectionA&& a, TCollectionB&& b, TLess Less, TCallback Callback)
{
	// To make this universal for const and non-const
	using TIteratorA = decltype(a.begin());
	using TIteratorB = decltype(b.begin());

	TIteratorA ItCurrA = a.begin();
	TIteratorB ItCurrB = b.begin();
	bool IsEndA = (ItCurrA == a.cend());
	bool IsEndB = (ItCurrB == b.cend());
	while (!IsEndA || !IsEndB)
	{
		TIteratorA ItA;
		TIteratorB ItB;
		if (IsEndB || (!IsEndA && Less(*ItCurrA, *ItCurrB)))
		{
			ItA = ItCurrA++;
			ItB = b.end();
			IsEndA = (ItCurrA == a.cend());
		}
		else if (IsEndA || (!IsEndB && Less(*ItCurrB, *ItCurrA)))
		{
			ItA = a.end();
			ItB = ItCurrB++;
			IsEndB = (ItCurrB == b.cend());
		}
		else // equal
		{
			ItA = ItCurrA++;
			ItB = ItCurrB++;
			IsEndA = (ItCurrA == a.cend());
			IsEndB = (ItCurrB == b.cend());
		}

		if constexpr (std::is_invocable_r_v<bool, TCallback, TIteratorA, TIteratorB>)
		{
			if (!Callback(ItA, ItB)) return;
		}
		else if constexpr (std::is_invocable_r_v<void, TCallback, TIteratorA, TIteratorB>)
		{
			Callback(ItA, ItB);
		}
		else static_assert(false, "Callback must accept iterators to a & b and return void or bool");
	}
}
//---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' and 'b'. When elements with the same key
// exist in both sets, the callback is invoked for them once with both elements valid.
template<typename TCollection, typename TCallback>
inline void SortedUnion(const TCollection& a, const TCollection& b, TCallback Callback)
{
	SortedUnion(a, b, a.value_comp(), std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

template<typename T, typename TCallback>
inline void SetDifference(const std::set<T>& a, const std::set<T>& b, TCallback Callback)
{
	SortedDifference(a, b, std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

template<typename T, typename TCallback>
inline void SetDifference(const std::unordered_set<T>& a, const std::unordered_set<T>& b, TCallback Callback)
{
	for (const auto& Value : a)
		if (b.find(Value) == b.cend())
			Callback(Value);
}
//---------------------------------------------------------------------

}
