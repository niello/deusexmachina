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

	constexpr static inline TIndex RED = 1;
	constexpr static inline TIndex BLACK = 0;
	constexpr static inline size_t LEFT = 0;
	constexpr static inline size_t RIGHT = 1;

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

	TIndex TreeMinNode(TIndex SubtreeRootIndex) const noexcept
	{
		if (SubtreeRootIndex >= _Data.size()) return INVALID_INDEX;

		while (_Data[SubtreeRootIndex]._Child[LEFT] != INVALID_INDEX)
			SubtreeRootIndex = _Data[SubtreeRootIndex]._Child[LEFT];

		return SubtreeRootIndex;
	}

	TIndex TreeMaxNode(TIndex SubtreeRootIndex) const noexcept
	{
		if (SubtreeRootIndex >= _Data.size()) return INVALID_INDEX;

		while (_Data[SubtreeRootIndex]._Child[RIGHT] != INVALID_INDEX)
			SubtreeRootIndex = _Data[SubtreeRootIndex]._Child[RIGHT];

		return SubtreeRootIndex;
	}

	TIndex TreeNextNode(TIndex Index) const noexcept
	{
		if (Index >= _Data.size()) return INVALID_INDEX;

		auto& Node = _Data[Index];
		if (Node._Child[RIGHT] != INVALID_INDEX) return TreeMinNode(Node._Child[RIGHT]);

		// Search for the unvisited right subtree in parents
		TIndex ParentIndex = Node._Parent;
		while (ParentIndex != INVALID_INDEX && _Data[ParentIndex]._Child[RIGHT] == Index)
		{
			Index = ParentIndex;
			ParentIndex = _Data[ParentIndex]._Parent;
		}

		return ParentIndex;
	}

	TIndex TreePrevNode(TIndex Index) const noexcept
	{
		if (Index >= _Data.size()) return _LastFreeIndex;

		auto& Node = _Data[Index];
		if (Node._Child[LEFT] != INVALID_INDEX) return TreeMaxNode(Node._Child[LEFT]);

		// Search for the unvisited left subtree in parents
		TIndex ParentIndex = Node._Parent;
		while (ParentIndex != INVALID_INDEX && _Data[ParentIndex]._Child[LEFT] == Index)
		{
			Index = ParentIndex;
			ParentIndex = _Data[ParentIndex]._Parent;
		}

		return ParentIndex;
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

	static DEM_FORCE_INLINE bool TreeNodeIsBlack(TIndex Index) noexcept { return (Index == INVALID_INDEX || _Data[Index]._Color == BLACK); }

	void TreeInsert(TIndex Index, TIndex Parent, bool AddToTheRight) noexcept
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
		while (!TreeNodeIsBlack(CurrParentIndex))
		{
			//!!!DBG TMP! Check grandparent existence. Is an invariant?
			n_assert_dbg(_Data[CurrParentIndex]._Parent != INVALID_INDEX);

			auto& CurrParent = _Data[CurrParentIndex];
			const auto CurrGrandParentIndex = CurrParent._Parent;
			auto& CurrGrandParent = _Data[CurrGrandParentIndex];

			const auto Dir = (CurrParentIndex == CurrGrandParent._Child[LEFT]) ? LEFT : RIGHT;
			const auto OppositeDir = 1 - Dir;

			const auto ParentSiblingIndex = CurrGrandParent._Child[OppositeDir];
			if (TreeNodeIsBlack(ParentSiblingIndex))
			{
				// Parent is red but its sibling is black. Make tree rotations and push red up.
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
			else
			{
				// Both of grandparent's children are red. Make them black and push red up.
				CurrParent._Color = BLACK;
				_Data[ParentSiblingIndex]._Color = BLACK;
				CurrGrandParent._Color = RED;
				CurrIndex = CurrGrandParentIndex;
			}

			CurrParentIndex = _Data[CurrIndex]._Parent;
		}

		// Ensure that the root is always black
		_Data[_TreeRootIndex]._Color = BLACK;
	}

	// NB: this method assumes that Index is a valid tree node index
	void TreeRemove(TIndex Index) noexcept
	{
		TIndex SubtreeIndex;
		TIndex ParentIndex;
		auto& Node = _Data[Index];
		if (Node._Child[LEFT] == INVALID_INDEX || Node._Child[RIGHT] == INVALID_INDEX)
		{
			// A single subtree, relink it to the parent of the erased node
			SubtreeIndex = (Node._Child[LEFT] == INVALID_INDEX) ? Node._Child[RIGHT] : Node._Child[LEFT];
			ParentIndex = Node._Parent;
			if (SubtreeIndex != INVALID_INDEX)
				_Data[SubtreeIndex]._Parent = ParentIndex;

			if (_TreeRootIndex == Index)
				_TreeRootIndex = SubtreeIndex;
			else if (_Data[ParentIndex]._Child[LEFT] == Index)
				_Data[ParentIndex]._Child[LEFT] = SubtreeIndex;
			else
				_Data[ParentIndex]._Child[RIGHT] = SubtreeIndex;

			// Update cached min-max values
			if (_FirstFreeIndex == Index)
				_FirstFreeIndex = (SubtreeIndex != INVALID_INDEX) ? TreeMinNode(SubtreeIndex) : ParentIndex;
			if (_LastFreeIndex == Index)
				_LastFreeIndex = (SubtreeIndex != INVALID_INDEX) ? TreeMaxNode(SubtreeIndex) : ParentIndex;
		}
		else
		{
			// Two subtrees, lift up the successor
			const auto SubtreeParentIndex = TreeNextNode(Index);
			auto& SuccessorNode = _Data[SubtreeParentIndex];
			SubtreeIndex = SuccessorNode._Child[RIGHT];

			// Relink the left subtree of the erased node as the left subtree to the successor
			_Data[Node._Child[LEFT]]._Parent = SubtreeParentIndex;
			SuccessorNode._Child[LEFT] = Node._Child[LEFT];

			if (SubtreeParentIndex == Node._Child[RIGHT])
			{
				// A successor is a direct child of the erased node
				ParentIndex = SubtreeParentIndex;
			}
			else
			{
				// Lift the right subtree of the successor as the left subtree of its parent
				ParentIndex = SuccessorNode._Parent;
				if (SubtreeIndex != INVALID_INDEX)
					_Data[SubtreeIndex]._Parent = ParentIndex;

				_Data[ParentIndex]._Child[LEFT] = SubtreeIndex;

				// Relink the right subtree of the erased node as the right subtree to the successor
				_Data[Node._Child[RIGHT]]._Parent = SubtreeParentIndex;
				SuccessorNode._Child[RIGHT] = Node._Child[RIGHT];
			}

			// Link the successor to the parent of the erased node
			if (_TreeRootIndex == Index)
				_TreeRootIndex = SubtreeParentIndex;
			else if (_Data[Node._Parent]._Child[LEFT] == Index)
				_Data[Node._Parent]._Child[LEFT] = SubtreeParentIndex;
			else
				_Data[Node._Parent]._Child[RIGHT] = SubtreeParentIndex;

			SuccessorNode._Parent = Node._Parent;

			// Exchange colors of the erased node and the successor linked to its place
			std::swap(SuccessorNode._Color, Node._Color);
		}

		// If the erasing a black node, the tree must be rebalanced
		if (Node._Color == BLACK)
		{
			while (SubtreeIndex != _TreeRootIndex && _Data[SubtreeIndex]._Color == BLACK)
			{
				auto& ParentNode = _Data[ParentIndex];

				// Determine a subtree that requires rebalancing
				const auto Dir = (SubtreeIndex == ParentNode._Child[LEFT]) ? LEFT : RIGHT;
				const auto OppositeDir = 1 - Dir;

				auto SiblingIndex = ParentNode._Child[OppositeDir];
				auto pSiblingNode = &_Data[SiblingIndex];
				if (pSiblingNode->_Color == RED)
				{
					// Propagate red up from the sibling subtree
					pSiblingNode->_Color = BLACK;
					ParentNode._Color = RED;
					Rotate(ParentIndex, Dir);
					SiblingIndex = ParentNode._Child[OppositeDir];
					pSiblingNode = &_Data[SiblingIndex];
				}

				auto SiblingChildDir = pSiblingNode->_Child[Dir];
				auto SiblingChildOpposite = pSiblingNode->_Child[OppositeDir];
				const bool SiblingChildOppositeIsBlack = TreeNodeIsBlack(SiblingChildOpposite);
				if (SiblingChildOppositeIsBlack && TreeNodeIsBlack(SiblingChildDir))
				{
					// When both children are black, the node itself is painted red
					pSiblingNode->_Color = RED;
					SubtreeIndex = ParentIndex;
				}
				else
				{
					if (SiblingChildOppositeIsBlack)
					{
						// Propagate red up from our subtree's child
						_Data[SiblingChildDir]._Color = BLACK;
						pSiblingNode->_Color = RED;
						Rotate(SiblingIndex, OppositeDir);
						SiblingIndex = ParentNode._Child[OppositeDir];
						pSiblingNode = &_Data[SiblingIndex];
					}

					// Finish rebalancing
					pSiblingNode->_Color = ParentNode._Color;
					ParentNode._Color = BLACK;
					_Data[pSiblingNode->_Child[OppositeDir]]._Color = BLACK;
					Rotate(ParentIndex, Dir);
					break;
				}

				ParentIndex = _Data[SubtreeIndex]._Parent;
			}

			_Data[SubtreeIndex]._Color = BLACK;
		}
	}

	TIndex AllocateFirstFreeCell()
	{
		const auto InternalSize = static_cast<TIndex>(_Data.size());
		if (_FirstFreeIndex >= InternalSize)
		{
			if (InternalSize == MAX_CAPACITY)
			{
				n_assert2(false, "CSparseArray2 max capacity exceeded");
				return INVALID_INDEX;
			}

			_Data.push_back();
			return InternalSize;
		}

		const auto FreeIndex = _FirstFreeIndex;
		TreeRemove(FreeIndex);
		return FreeIndex;
	}

	// Returns the next busy cell index
	TIndex FreeAllocatedCell(TIndex Index) noexcept
	{
		// If erasing at the tail, pop it from _Data instead of adding to the tree
		if (Index + 1 == _Data.size() && Index != _LastFreeIndex)
		{
			--_Size;
			UnsafeAt(Index).~T();
			_Data.pop_back();
			return INVALID_INDEX;
		}

		// Silently skip inserting an already free node
		const auto Found = TreeFindLowerBound(Index);
		if (Found.Node != Index)
		{
			--_Size;
			UnsafeAt(Index).~T();
			TreeInsert(Index, Found.Parent, Found.IsRightChild);
		}

		// Given the Index is free, find next busy cell (a gap between free cells)
		TIndex NextCell = Index + 1;
		TIndex NextFree = TreeNextNode(Index);
		while (NextFree != INVALID_INDEX && NextFree == NextCell)
		{
			NextCell = NextFree + 1;
			NextFree = TreeNextNode(NextFree);
		}

		return (NextCell < _Data.size()) ? NextCell : INVALID_INDEX;
	}

public:

	CSparseArray2() = default;

	void reserve(TIndex NewCapacity) { _Data.reserve(NewCapacity); }

	bool is_filled(TIndex Index) const noexcept { return TreeFindLowerBound(Index).Node != Index; }

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
