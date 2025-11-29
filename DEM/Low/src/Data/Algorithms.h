#pragma once
#include <Data/FunctionTraits.h>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>

// Utility algorithms and wrappers over std algorithms

namespace DEM::Algo
{

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, UPTR Index)
{
	if (Index >= Self.size()) return false;
	if (Index < Self.size() - 1) std::swap(Self[Index], Self[Self.size() - 1]);
	Self.pop_back();
	return true;
}
//---------------------------------------------------------------------

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, typename std::vector<T>::iterator It)
{
	if (It == Self.cend()) return false;
	if (It != --Self.cend())
		std::swap(*It, Self.back());
	Self.pop_back();
	return true;
}
//---------------------------------------------------------------------

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, const T& Value)
{
	return VectorFastErase(Self, std::find(Self.begin(), Self.end(), Value));
}
//---------------------------------------------------------------------

// The preferred one for almost sorted collections
template<typename TCollection>
void InsertionSort(TCollection& Data)
{
	const size_t Size = Data.size();
	for (size_t i = 1; i < Size; ++i)
	{
		size_t j = i - 1;
		if (Data[i] < Data[j])
		{
			auto Value = std::move(Data[i]);
			do
			{
				Data[j + 1] = std::move(Data[j]);
				if (j == 0) break;
				--j;
			}
			while (Value < Data[j]);

			Data[j + 1] = std::move(Value);
		}
	}
}
//---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' that do not appear in 'b'
template<typename TCollection, typename TLess, typename TCallback>
inline void SortedDifference(const TCollection& a, const TCollection& b, TLess Less, TCallback Callback)
{
	auto ItCurrA = a.cbegin();
	auto ItCurrB = b.cbegin();
	while (ItCurrA != a.cend() && ItCurrB != b.cend())
	{
		if (Less(*ItCurrA, *ItCurrB))
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
			if (!Less(*ItCurrB, *ItCurrA)) ++ItCurrA;
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

HAS_METHOD_WITH_SIGNATURE_TRAIT(value_comp);

// Calls a Callback with iterators to elements from 'a' and 'b'. When elements with the same key
// exist in both sets, the callback is invoked for them once with both elements valid. Otherwise
// at least one of iterators is valid. This works similar to SQL "outer join".
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
		else if (IsEndA || Less(*ItCurrB, *ItCurrA))
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

template<typename TCollection, typename TCallback>
DEM_FORCE_INLINE void SortedUnion(const TCollection& a, const TCollection& b, TCallback Callback)
{
	if constexpr (has_method_with_signature_value_comp_v<TCollection, void()>)
		SortedUnion(a, b, a.value_comp(), std::forward<TCallback>(Callback));
	else
		SortedUnion(a, b, std::less<typename TCollection::value_type>{}, std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' and 'b'. When elements with the same key
// exist in both sets, the callback is invoked for them once with both elements valid. If element exists
// only in 'a', the first iterator is valid and the second is not. Otherwise a callback is not called.
// This works similar to SQL "left join".
template<typename TCollectionA, typename TCollectionB, typename TLess, typename TCallback>
DEM_FORCE_INLINE void SortedLeftJoin(TCollectionA&& a, TCollectionB&& b, TLess Less, TCallback Callback)
{
	// To make this universal for const and non-const
	using TIteratorA = decltype(a.begin());
	using TIteratorB = decltype(b.begin());

	TIteratorA ItCurrA = a.begin();
	TIteratorB ItCurrB = b.begin();
	bool IsEndB = (ItCurrB == b.cend());
	while (ItCurrA != a.cend())
	{
		TIteratorA ItA;
		TIteratorB ItB;
		if (IsEndB || Less(*ItCurrA, *ItCurrB))
		{
			ItA = ItCurrA++;
			ItB = b.end();
		}
		else if (Less(*ItCurrB, *ItCurrA))
		{
			++ItCurrB;
			IsEndB = (ItCurrB == b.cend());
			continue;
		}
		else // equal
		{
			ItA = ItCurrA++;
			ItB = ItCurrB++;
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

template<typename TCollection, typename TCallback>
DEM_FORCE_INLINE void SortedLeftJoin(const TCollection& a, const TCollection& b, TCallback Callback)
{
	if constexpr (has_method_with_signature_value_comp_v<TCollection, void()>)
		SortedLeftJoin(a, b, a.value_comp(), std::forward<TCallback>(Callback));
	else
		SortedLeftJoin(a, b, std::less<typename TCollection::value_type>{}, std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

// Calls a Callback with iterators to elements from 'a' and 'b'. When elements with the same key
// exist in both sets, the callback is invoked for them once with both elements valid. Otherwise
// a callback is not called. This works similar to SQL "inner join".
template<typename TCollectionA, typename TCollectionB, typename TLess, typename TCallback>
DEM_FORCE_INLINE void SortedInnerJoin(TCollectionA&& a, TCollectionB&& b, TLess Less, TCallback Callback)
{
	// To make this universal for const and non-const
	using TIteratorA = decltype(a.begin());
	using TIteratorB = decltype(b.begin());

	TIteratorA ItCurrA = a.begin();
	TIteratorB ItCurrB = b.begin();
	while (ItCurrA != a.cend() && ItCurrB != b.cend())
	{
		if (Less(*ItCurrA, *ItCurrB))
		{
			++ItCurrA;
		}
		else if (Less(*ItCurrB, *ItCurrA))
		{
			++ItCurrB;
		}
		else // equal
		{
			if constexpr (std::is_invocable_r_v<bool, TCallback, TIteratorA, TIteratorB>)
			{
				if (!Callback(ItCurrA, ItCurrB)) return;
			}
			else if constexpr (std::is_invocable_r_v<void, TCallback, TIteratorA, TIteratorB>)
			{
				Callback(ItCurrA, ItCurrB);
			}
			else static_assert(false, "Callback must accept iterators to a & b and return void or bool");

			++ItCurrA;
			++ItCurrB;
		}
	}
}
//---------------------------------------------------------------------

template<typename TCollection, typename TCallback>
DEM_FORCE_INLINE void SortedInnerJoin(const TCollection& a, const TCollection& b, TCallback Callback)
{
	if constexpr (has_method_with_signature_value_comp_v<TCollection, void()>)
		SortedInnerJoin(a, b, a.value_comp(), std::forward<TCallback>(Callback));
	else
		SortedInnerJoin(a, b, std::less<typename TCollection::value_type>{}, std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

template<typename... T, typename TCallback>
inline void MapDifference(const std::map<T...>& a, const std::map<T...>& b, TCallback Callback)
{
	SortedDifference(a, b, [](const auto& va, const auto& vb) { return va.first < vb.first; }, std::forward<TCallback>(Callback));
}
//---------------------------------------------------------------------

template<typename T, typename TCallback>
inline void SetDifference(const std::set<T>& a, const std::set<T>& b, TCallback Callback)
{
	SortedDifference(a, b, std::less<T>{}, std::forward<TCallback>(Callback));
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

template<typename... T>
void InplaceIntersection(std::set<T...>& a, const std::set<T...>& b)
{
	auto ItA = a.begin();
	auto ItB = b.cbegin();
	while (ItA != a.end() && ItB != b.cend())
	{
		if (*ItA < *ItB)
		{
			ItA = a.erase(ItA);
		}
		else
		{
			if (!(*ItB < *ItA)) ++ItA;
			++ItB;
		}
	}

	a.erase(ItA, a.end());
}
//---------------------------------------------------------------------

template<typename... T>
void InplaceDifference(std::set<T...>& a, const std::set<T...>& b)
{
	auto ItA = a.begin();
	auto ItB = b.cbegin();
	while (ItA != a.end() && ItB != b.cend())
	{
		if (*ItA < *ItB)
		{
			++ItA;
		}
		else if (*ItB < *ItA)
		{
			++ItB; // for big b could also be: b.lower_bound(*ItA);
		}
		else
		{
			ItA = a.erase(ItA);
			++ItB;
		}
	}
}
//---------------------------------------------------------------------

}
