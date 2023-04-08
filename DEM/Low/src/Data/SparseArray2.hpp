#pragma once
#include <vector>

// Array which doesn't shift when elements are deleted. Instead free cells are remembered
// and reused on subsequent insertions. This implementation preserves ordering of free
// slots by building a red-black tree from them, and provides O(1) insertion to
// highest or lowest free index. As long as sizeof(T) >= sizeof(3 * TIndex), this implementation
// consumes no additional O(n) storage compared to the regular vector, and keeps O(n) forward
// and backward iteration complexity. Allocating at the lowest free index by default provides
// the cheapest possible Compact() with minimized amount of element moves, and also improves
// CPU cache utilization.

namespace Data
{

template<typename T, typename TIndex = size_t>
class CSparseArray2
{
	static_assert(std::is_unsigned_v<TIndex>&& std::is_integral_v<TIndex> && !std::is_same_v<TIndex, bool>,
		"DEM::Data::CSparseArray2 > TIndex must be an unsigned integral type other than bool");

public:

	constexpr static inline auto INVALID_INDEX = std::numeric_limits<TIndex>().max();
	constexpr static inline auto INDEX_BITS = sizeof(TIndex) * 8 - 1; // One bit is reserved for a tree node color
	constexpr static inline auto MAX_CAPACITY = static_cast<TIndex>(1 << INDEX_BITS);

protected:

	constexpr static inline bool RED = true;
	constexpr static inline bool BLACK = false;
	constexpr static inline size_t LEFT = 0;
	constexpr static inline size_t RIGHT = 0;

	union CCell
	{
		alignas(T) std::byte ElementStorage[sizeof(T)];

		struct // Could store a single uint64 = 21 bit * 3 indices + 1 bit of color, max 2M elements in the array
		{
			TIndex _Child[2];
			TIndex _Parent : INDEX_BITS;
			TIndex _Color : 1;
		};
	};

	using TUnderlyingVector = std::vector<CCell>;

	class CIterator
	{
	private:

		// curr stop = _FirstFreeIndex
		// iterate to min(curr stop, _Data.size)
		//   process elements
		// skip curr stop, read new curr stop (next free cell) and repeat
		// iterator could consist of vector iterator and index of next free element

		typename TUnderlyingVector::iterator It;

	public:

		using pointer = T*; // TODO: const pointer for const iterator
		using reference = T&; // TODO: const reference for const iterator

		reference operator*() const noexcept; //{ return _Ptr->_Myval; }
		pointer operator->() const noexcept; //{ return pointer_traits<pointer>::pointer_to(**this); }
	};

public:

	using size_type = TIndex;
	using value_type = T;
	using iterator = CIterator;
	using const_iterator = CIterator; // FIXME: constant version

protected:

	TUnderlyingVector _Data;

	TIndex _FirstFreeIndex = INVALID_INDEX;
	TIndex _LastFreeIndex = INVALID_INDEX;
	TIndex _TreeRootIndex = INVALID_INDEX;

	TIndex _Size = 0;

	T& UnsafeAt(TIndex Index) noexcept { return *reinterpret_cast<T*>(&_Data[Index].ElementStorage); }
	const T& UnsafeAt(TIndex Index) const noexcept { return *reinterpret_cast<T*>(&_Data[Index].ElementStorage); }

	struct CBound
	{
		TIndex Parent;
		TIndex Node;
		bool IsRightChild;
	};

	CBound TreeFindLowerBound(TIndex Index) const noexcept
	{
		CBound Result{ _TreeRootIndex, INVALID_INDEX, true };
		TIndex CurrNode = _TreeRootIndex;
		while (CurrNode != INVALID_INDEX)
		{
			Result.Parent = CurrNode;
			Result.IsRightChild = (CurrNode < Index);
			if (Result.IsRightChild)
			{
				CurrNode = _Data[CurrNode]._Child[RIGHT];
			}
			else
			{
				Result.Node = CurrNode;
				CurrNode = _Data[CurrNode]._Child[LEFT];
			}
		}

		return Result;
	}

	void TreeRotate(TIndex Index, size_t Dir) noexcept
	{
		const auto OppositeDir = 1 - Dir;

		auto& Node = _Data[Index];

		const auto ChildNodeIndex = Node._Child[OppositeDir];
		auto& ChildNode = _Data[ChildNodeIndex];
		const auto GrandChildNodeIndex = ChildNode._Child[Dir];

		// Turn a grandchild into a child
		Node._Child[OppositeDir] = GrandChildNodeIndex;
		if (GrandChildNodeIndex != INVALID_INDEX)
			_Data[GrandChildNodeIndex]._Parent = Index;

		// Move a child into the node's place
		ChildNode._Parent = Node._Parent;
		if (Index == _TreeRootIndex)
			_TreeRootIndex = ChildNodeIndex;
		else if (Index == _Data[Node._Parent]._Child[Dir])
			_Data[Node._Parent]._Child[Dir] = ChildNodeIndex;
		else
			_Data[Node._Parent]._Child[OppositeDir] = ChildNodeIndex;

		// Make the node a child of its former child
		ChildNode._Child[Dir] = Index;
		Node._Parent = ChildNodeIndex;
	}

	void TreeInsert(TIndex Index, TIndex Parent, bool AddToTheRight)
	{
		auto& Node = _Data[Index];
		Node._Child[LEFT] = INVALID_INDEX;
		Node._Child[RIGHT] = INVALID_INDEX;
		Node._Parent = Parent;

		// Insert the first node into an empty tree
		if (Parent == INVALID_INDEX)
		{
			_FirstFreeIndex = Index;
			_LastFreeIndex = Index;
			_TreeRootIndex = Index;
			Node._Color = BLACK;
			return;
		}

		// Attach to the parent and update cached min-max values
		auto& ParentNode = _Data[Parent];
		if (AddToTheRight)
		{
			ParentNode._Child[RIGHT] = Index;
			if (Index > _LastFreeIndex) _LastFreeIndex = Index;
		}
		else
		{
			ParentNode._Child[LEFT] = Index;
			if (Index < _FirstFreeIndex) _FirstFreeIndex = Index;
		}

		// Paint the newly added node red
		Node._Color = RED;

		// Fix red-violations
		TIndex CurrIndex = Index;
		TIndex CurrParentIndex = Parent;
		while (CurrParentIndex != INVALID_INDEX && _Data[CurrParentIndex]._Color == RED)
		{
			//!!!DBG TMP! Check grandparent existence. Is an invariant?
			n_assert_dbg(_Data[CurrParentIndex]._Parent != INVALID_INDEX);

			auto& CurrParent = _Data[CurrParentIndex];
			const auto CurrGrandParentIndex = CurrParent._Parent;
			auto& CurrGrandParent = _Data[CurrGrandParentIndex];

			const auto Dir = (CurrParentIndex == CurrGrandParent._Child[LEFT]) ? LEFT : RIGHT;
			const auto OppositeDir = 1 - Dir;

			const auto ParentSiblingIndex = CurrGrandParent._Child[OppositeDir];
			if (ParentSiblingIndex != INVALID_INDEX && _Data[ParentSiblingIndex]._Color == RED)
			{
				// Both of grandparent's children are red. Make them black and push red up.
				CurrParent._Color = BLACK;
				_Data[ParentSiblingIndex]._Color = BLACK;
				CurrGrandParent._Color = RED;
				CurrIndex = CurrGrandParentIndex;
			}
			else
			{
				// Only one of grandparent's children is red. Make tree rotations and push red up.
				if (CurrIndex == CurrParent._Child[OppositeDir])
				{
					CurrIndex = CurrParentIndex;
					TreeRotate(CurrIndex, Dir);
				}

				auto& ParentAfterRotation = _Data[_Data[CurrIndex]._Parent];
				ParentAfterRotation._Color = BLACK;
				_Data[ParentAfterRotation._Parent]._Color = RED;
				TreeRotate(ParentAfterRotation._Parent, OppositeDir);
			}

			CurrParentIndex = _Data[CurrIndex]._Parent;
		}

		//!!!DBG TMP! Can be red here?
		n_assert_dbg(_Data[_TreeRootIndex]._Color == BLACK);

		// Ensure that the root is always black
		_Data[_TreeRootIndex]._Color = BLACK;
	}

	TIndex AllocateFirstFreeCell()
	{
		const auto InternalSize = _Data.size();
		if (_FirstFreeIndex >= InternalSize)
		{
			if (InternalSize == MAX_CAPACITY)
			{
				n_assert2(false, "CSparseArray2 max capacity exceeded");
				return INVALID_INDEX;
			}

			_Data.push_back();
			return static_cast<TIndex>(InternalSize);
		}

		const auto FreeIndex = _FirstFreeIndex;

		// TODO: remove _FirstFreeIndex from the tree "by iterator", update min or max or both if equal

		return FreeIndex;
	}

	// Returns the next busy cell index
	TIndex FreeAllocatedCell(TIndex Index) noexcept
	{
		// If erasing at the tail, pop it from _Data instead of adding to the tree
		if (Index == _Data.size() - 1 && Index != _LastFreeIndex)
		{
			--_Size;
			UnsafeAt(Index).~T();
			_Data.pop_back();
			return INVALID_INDEX;
		}

		// Silently skip an already inserted node. Should never happen.
		const auto Found = TreeFindLowerBound(Index);
		if (Found.Node == Index) return;

		--_Size;
		UnsafeAt(Index).~T();
		TreeInsert(Index, Found.Parent, Found.IsRightChild);

		//!!!TODO: find first node after Index not in the tree! Any index > _Data.size() is end().
		return XXX;
	}

public:

	CSparseArray2() = default;

	void reserve(TIndex NewCapacity) { _Data.reserve(NewCapacity); }

	bool is_filled(TIndex Index) const noexcept { return TreeFindLowerBound(Index).Node == Index; }

	iterator insert(const T& Value) { return emplace(Value); }
	iterator insert(T&& Value) { return emplace(std::move(Value)); }

	template <class... TArgs>
	iterator emplace(TArgs&&... ValueConstructionArgs)
	{
		auto Index = AllocateFirstFreeCell();
		if (Index == INVALID_INDEX) return end();
		++_Size;
		new (_Data[Index].ElementStorage) T(std::forward<TArgs>(ValueConstructionArgs)...);
		return iterator(Index);
	}

	iterator erase(const_iterator Where) noexcept { return erase(static_cast<TIndex>(std::distance(_Data.cbegin(), Where.It))); }
	iterator erase(TIndex Index) noexcept { return iterator(FreeAllocatedCell(Index)); }

	void clear()
	{
		for (auto it = begin(); it != end(); ++it)
			(*it).~T();

		_Data.clear();
		_FirstFreeIndex = INVALID_INDEX;
		_LastFreeIndex = INVALID_INDEX;
		_TreeRootIndex = INVALID_INDEX;
		_Size = 0;
	}

	void shrink_to_fit()
	{
		// TODO: shrink tail of free cells!
		// while (!_Data.empty() && _LastFreeIndex == _Data.size() - 1) remove _LastFreeIndex from the tree "by iterator"

		_Data.shrink_to_fit();
	}

	void compact()
	{
		// minimize swaps - iterate free cells forward and busy cells backwards
		// shrink after compacting
		// clear free tree
	}

	void compact_ordered()
	{
		// don't change order of valid elements - offset busy cells by running sum of holes
		// shrink after compacting
		// clear free tree
	}

	constexpr TIndex max_size() const noexcept { return MAX_CAPACITY; }
	TIndex size() const noexcept { return _Size; }
	TIndex sparse_size() const noexcept { return static_cast<TIndex>(_Data.size()); }
	TIndex capacity() const noexcept { return static_cast<TIndex>(_Data.capacity()); }
	bool empty() const noexcept { return !_Size; }

	//reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	//const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	//reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	//const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
	//const_iterator cbegin() const noexcept { return begin(); }
	//const_iterator cend() const noexcept { return end(); }
	//const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	//const_reverse_iterator crend() const noexcept { return rend(); }

	T& operator[](TIndex Index) noexcept { return UnsafeAt(Index); }
	const T& operator[](const TIndex Index) const noexcept { return UnsafeAt(Index); }

	T& at(const TIndex Index) { n_assert(is_filled(Index)); return UnsafeAt(Index); }
	const T& at(const TIndex Index) const { n_assert(is_filled(Index)); return UnsafeAt(Index); }
};

}
