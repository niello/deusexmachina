#pragma once
#include <vector>
#include <algorithm>

// An STL styled associative container which preserves an insertion order

// TODO: the implementation is very lazy and lacks lots of standard things! Extend when needed!

template<typename TKey, typename TValue, class TAlloc = std::allocator<std::pair<TKey, TValue>>>
class CFixedOrderMap : public std::vector<std::pair<TKey, TValue>, TAlloc>
{
public:

	using Base = std::vector<std::pair<TKey, TValue>>;
	using key_type = TKey;
	using mapped_type = TValue;
	using value_type = std::pair<TKey, TValue>;
	using allocator_type = typename Base::allocator_type;
	using size_type = typename Base::size_type;
	using difference_type = typename Base::difference_type;
	using pointer = typename Base::pointer;
	using const_pointer = typename Base::const_pointer;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = typename Base::iterator;
	using const_iterator = typename Base::const_iterator;
	using reverse_iterator = typename Base::reverse_iterator;
	using const_reverse_iterator = typename Base::const_reverse_iterator;

	template<class... Args>
	std::pair<iterator, bool> emplace(Args&&... args)
	{
		value_type Record(std::forward<Args>(args)...);
		auto It = find(Record.first);
		if (It != cend()) return { It, false };
		emplace_back(std::move(Record));
		return { std::prev(end()), true };
	}

	template <class MappedType>
	std::pair<iterator, bool> insert_or_assign(key_type&& Key, MappedType&& Value)
	{
		auto It = find(Key);
		if (It != cend())
		{
			It->second = std::forward<MappedType>(Value);
			return { It, false };
		}
		emplace_back(std::forward<key_type>(Key), std::forward<MappedType>(Value));
		return { std::prev(end()), true };
	}

	size_type erase(const key_type& Key) noexcept
	{
		const size_type SizeBefore = size();
		Base::erase(std::remove_if(begin(), end(), [&Key](const auto& Pair) { return Pair.first == Key; }), end());
		return SizeBefore - size();
	}

	[[nodiscard]] iterator find(const key_type& Key)
	{
		for (auto It = begin(); It != end(); ++It)
			if (It->first == Key)
				return It;
		return end();
	}

	[[nodiscard]] const_iterator find(const key_type& Key) const
	{
		for (auto It = cbegin(); It != cend(); ++It)
			if (It->first == Key)
				return It;
		return cend();
	}
};
