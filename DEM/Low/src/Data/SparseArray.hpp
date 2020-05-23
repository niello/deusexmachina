#pragma once
#include <type_traits>
#include <vector>

// Array which doesn't shift when elements are deleted. Instead free cells are remembered
// and reused on subsequent insertions. Provides O(1) insertion and deletion without data
// movement. Elements aren't guaranteed to be inserted in order.

// TODO: vacuum (invalidate indices), shrink_to_fit (keep indices valid)

namespace Data
{

template<typename T, typename TIndex = size_t>
class CSparseArray
{
	static_assert(std::is_unsigned_v<TIndex> && std::is_integral_v<TIndex> && !std::is_same_v<TIndex, bool>,
		"CHandleArray > TIndex must be an unsigned integral type other than bool");

protected:

	struct CCell
	{
		T    Value;
		bool Free = false; // For faster iteration and indexed access control
	};

	std::vector<CCell> _Data;
	std::vector<U32>   _FreeIndices;

public:

	constexpr static inline auto INVALID_INDEX = std::numeric_limits<TIndex>().max();
	constexpr static inline auto MAX_CAPACITY = std::numeric_limits<TIndex>().max();

	CSparseArray() = default;
	CSparseArray(size_t InitialCapacity) { _Data.reserve(std::min(InitialCapacity, MAX_CAPACITY)); }

	TIndex insert(T&& Value = T{})
	{
		if (_FreeIndices.empty())
		{
			const TIndex Index = static_cast<TIndex>(_Data.size());
			if (Index < INVALID_INDEX)
			{
				_Data.push_back(CCell{ std::move(Value), false });
				return Index;
			}
		}
		else
		{
			const TIndex Index = _FreeIndices.back();
			_FreeIndices.pop_back();

			auto& Cell = _Data[Index];
			n_assert_dbg(Cell.Free);
			Cell.Value = std::move(Value);
			Cell.Free = false;
			return Index;
		}

		return INVALID_INDEX;
	}

	TIndex insert(const T& Value)
	{
		if (_FreeIndices.empty())
		{
			const TIndex Index = static_cast<TIndex>(_Data.size());
			if (Index < INVALID_INDEX)
			{
				_Data.push_back(CCell{ Value, false });
				return Index;
			}
		}
		else
		{
			const TIndex Index = _FreeIndices.back();
			_FreeIndices.pop_back();

			auto& Cell = _Data[Index];
			n_assert_dbg(Cell.Free);
			Cell.Value = Value;
			Cell.Free = false;
			return Index;
		}

		return INVALID_INDEX;
	}

	template<typename... TArgs>
	TIndex emplace(TArgs&&... Args)
	{
		return insert(std::forward<TArgs>(Args)...);
	}

	void erase(TIndex Index)
	{
		if (Index >= INVALID_INDEX) return;

		auto& Cell = _Data[Index];
		n_assert(!Cell.Free);

		// Clear the record value to default, this clears freed object resources
		Cell.Value = {};
		Cell.Free = true;
		_FreeIndices.push_back(Index);
	}

	void clear()
	{
		_Data.clear();
		_FreeIndices.clear();
	}

	size_t size() const { return _Data.size() - _FreeIndices.size(); }
	bool   empty() const { return _Data.size() == _FreeIndices.size(); }

	T& operator [](TIndex Index) { n_assert_dbg(!_Data[Index].Free); return _Data[Index].Value; }
	const T& operator [](TIndex Index) const { n_assert_dbg(!_Data[Index].Free); return _Data[Index].Value; }

// *** Iterators ***

protected:

	// Iterates through allocated records only
	template<typename internal_it>
	class iterator_tpl
	{
	private:

		internal_it _It;
		internal_it _ItEnd; // Otherwise we don't know when to stop advancing through free records

	public:

		using iterator_category = std::bidirectional_iterator_tag;

		using value_type = typename internal_it::value_type;
		using difference_type = typename internal_it::difference_type;
		using pointer = typename internal_it::pointer;
		using reference = typename internal_it::reference;

		iterator_tpl() = default;
		iterator_tpl(internal_it It, internal_it ItEnd) : _It(It), _ItEnd(ItEnd) {}
		iterator_tpl(const iterator_tpl& It) = default;
		iterator_tpl& operator =(const iterator_tpl& It) = default;

		auto& operator *() const { return _It->Value; }
		auto* operator ->() const { return &_It->Value; }
		iterator_tpl& operator ++() { do ++_It; while (_It != _ItEnd && _It->Free); return *this; }
		iterator_tpl operator ++(int) { auto Tmp = *this; ++(*this); return Tmp; }
		iterator_tpl& operator --() { do --_It; while (_It != _ItEnd && _It->Free); return *this; }
		iterator_tpl operator --(int) { auto Tmp = *this; --(*this); return Tmp; }

		bool operator ==(const iterator_tpl& Right) const { return _It == Right._It; }
		bool operator !=(const iterator_tpl& Right) const { return _It != Right._It; }

		bool IsFree() const { return _It->Free; }
	};

public:

	using iterator = iterator_tpl<typename std::vector<CCell>::iterator>;
	using const_iterator = iterator_tpl<typename std::vector<CCell>::const_iterator>;

	const_iterator cbegin() const
	{
		const_iterator It(_Data.cbegin(), _Data.cend());
		const_iterator ItEnd(_Data.cend(), _Data.cend());
		while (It != ItEnd && It.IsFree()) ++It;
		return It;
	}

	iterator begin()
	{
		iterator It(_Data.begin(), _Data.end());
		iterator ItEnd(_Data.end(), _Data.end());
		while (It != ItEnd && It.IsFree()) ++It;
		return It;
	}

	const_iterator   begin() const { return cbegin(); }
	const_iterator   cend() const { return const_iterator(_Data.cend(), _Data.cend()); }
	iterator         end() { return iterator(_Data.end(), _Data.end()); }
	const_iterator   end() const { return cend(); }
};

}
