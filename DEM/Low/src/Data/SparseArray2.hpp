#pragma once
#include <System/System.h> // DEM_FORCE_INLINE, n_assert_dbg
#include <vector>

// Array which doesn't shift when elements are deleted. Instead free cells are remembered
// and reused on subsequent insertions. This implementation preserves ordering of free slots
// by index by building a red-black tree from them, and provides amortized O(1) insertion to
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
	static_assert(std::is_unsigned_v<TIndex> && std::is_integral_v<TIndex> && !std::is_same_v<TIndex, bool>,
		"DEM::Data::CSparseArray2 > TIndex must be an unsigned integral type other than bool");

public:

	constexpr static inline auto INDEX_BITS = sizeof(TIndex) * 8 - 1; // One bit is reserved for a tree node color
	constexpr static inline auto INVALID_INDEX = static_cast<TIndex>(1 << INDEX_BITS) - 1;
	constexpr static inline auto MAX_CAPACITY = static_cast<TIndex>(1 << INDEX_BITS);

protected:

	constexpr static inline TIndex RED = 1;
	constexpr static inline TIndex BLACK = 0;
	constexpr static inline size_t LEFT = 0;
	constexpr static inline size_t RIGHT = 1;

	union CCell
	{
		alignas(T) std::byte _ElementStorage[sizeof(T)];

		struct // Could store a single uint64 = 21 bit * 3 indices + 1 bit of color, max 2M elements in the array
		{
			TIndex _Child[2];
			TIndex _Parent : INDEX_BITS;
			TIndex _Color : 1;
		};
	};

	struct CBound
	{
		TIndex Parent;
		TIndex Node;
		bool IsRightChild;
	};

	class CConstantIterator
	{
	protected:

		friend class CSparseArray2<T, TIndex>;

		const CSparseArray2<T, TIndex>* _pOwner = nullptr;
		TIndex _CurrIndex = INVALID_INDEX;
		TIndex _PrevFreeIndex = INVALID_INDEX;
		TIndex _NextFreeIndex = INVALID_INDEX;

		void RewindToNextBusyCell()
		{
			while (_CurrIndex < _pOwner->sparse_size() && _CurrIndex == _NextFreeIndex)
			{
				++_CurrIndex;
				_PrevFreeIndex = _NextFreeIndex;
				_NextFreeIndex = _pOwner->TreeNextNode(_NextFreeIndex);
			}

			// Use INVALID_INDEX for all end() iterators to protect from sparse_size() changes during iteration
			if (_CurrIndex >= _pOwner->sparse_size()) _CurrIndex = INVALID_INDEX;
		}

		void RewindToPrevBusyCell()
		{
			while (_CurrIndex > 0 && _CurrIndex == _PrevFreeIndex)
			{
				--_CurrIndex;
				_NextFreeIndex = _PrevFreeIndex;
				_PrevFreeIndex = _pOwner->TreePrevNode(_PrevFreeIndex);
			}

			if (_CurrIndex == 0 && _PrevFreeIndex == 0) _CurrIndex = INVALID_INDEX;
		}

	public:

		CConstantIterator() = default;

		CConstantIterator(TIndex Index, const CSparseArray2<T, TIndex>& Owner)
			: _pOwner(&Owner)
			, _CurrIndex(std::min(Index, Owner.sparse_size()))
		{
			if (_CurrIndex == 0)
			{
				_PrevFreeIndex = INVALID_INDEX;
				_NextFreeIndex = Owner._FirstFreeIndex;
			}
			else if (_CurrIndex >= Owner.sparse_size())
			{
				_PrevFreeIndex = Owner._LastFreeIndex;
				_NextFreeIndex = INVALID_INDEX;
			}
			else
			{
				// Find the first free index not before the _CurrIndex
				_NextFreeIndex = Owner.TreeFindLowerBound(_CurrIndex).Node;
				if (_NextFreeIndex < _CurrIndex)
				{
					_PrevFreeIndex = _NextFreeIndex;
					_NextFreeIndex = Owner.TreeNextNode(_NextFreeIndex);
				}
				else
				{
					_PrevFreeIndex = Owner.TreePrevNode(_NextFreeIndex);
				}
			}

			// This will turn us into end() for an empty array
			RewindToNextBusyCell();
		}

		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = const value_type*;
		using reference = const value_type&;

		TIndex get_index() const noexcept { return _CurrIndex; }

		reference operator*() const noexcept { return _pOwner->UnsafeAt(_CurrIndex); }
		pointer operator->() const noexcept { return &_pOwner->UnsafeAt(_CurrIndex); }

		CConstantIterator& operator ++() noexcept
		{
			n_assert_dbg(_CurrIndex != INVALID_INDEX);
			++_CurrIndex;
			RewindToNextBusyCell();
			return *this;
		}

		CConstantIterator& operator --() noexcept
		{
			n_assert_dbg(_CurrIndex != 0);
			// Restore the index from INVALID_INDEX for reverse iterators to work
			if (_CurrIndex > _pOwner->sparse_size()) _CurrIndex = _pOwner->sparse_size();
			--_CurrIndex;
			RewindToPrevBusyCell();
			return *this;
		}

		CConstantIterator operator --(int) noexcept { CConstantIterator Tmp = *this; --(*this); return Tmp; }
		CConstantIterator operator ++(int) noexcept { CConstantIterator Tmp = *this; ++(*this); return Tmp; }

		bool operator ==(const CConstantIterator& Other) const noexcept { /*n_assert_dbg(_pOwner == Other._pOwner);*/ return _CurrIndex == Other._CurrIndex; }
		bool operator !=(const CConstantIterator& Other) const noexcept { return !(*this == Other); }
	};

	class CIterator : public CConstantIterator
	{
	public:

		using pointer = value_type*;
		using reference = value_type&;

		using CConstantIterator::CConstantIterator;

		reference operator*() const noexcept { return const_cast<reference>(CConstantIterator::operator *()); }
		pointer operator->() const noexcept { return const_cast<pointer>(CConstantIterator::operator ->()); }

		CIterator& operator ++() noexcept { CConstantIterator::operator ++(); return *this; }
		CIterator& operator --() noexcept { CConstantIterator::operator --(); return *this; }
		CIterator operator --(int) noexcept { CIterator Tmp = *this; CConstantIterator::operator --(); return Tmp; }
		CIterator operator ++(int) noexcept { CIterator Tmp = *this; CConstantIterator::operator ++(); return Tmp; }
	};

public:

	using size_type = TIndex;
	using value_type = T;
	using iterator = CIterator;
	using const_iterator = CConstantIterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

protected:

	std::vector<CCell> _Data;

	TIndex _FirstFreeIndex = INVALID_INDEX;
	TIndex _LastFreeIndex = INVALID_INDEX;
	TIndex _TreeRootIndex = INVALID_INDEX;

	TIndex _Size = 0;

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

	DEM_FORCE_INLINE bool TreeNodeIsBlack(TIndex Index) noexcept { return (Index == INVALID_INDEX || _Data[Index]._Color == BLACK); }

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

			// Exchange colors of the erased node and the successor linked to its place. NB: std::swap can't handle bit fields.
			const auto TmpColor = SuccessorNode._Color;
			SuccessorNode._Color = Node._Color;
			Node._Color = TmpColor;
		}

		// If the erasing a black node, the tree must be rebalanced
		if (Node._Color == BLACK && SubtreeIndex != INVALID_INDEX)
		{
			while (SubtreeIndex != _TreeRootIndex && _Data[SubtreeIndex]._Color == BLACK)
			{
				auto& ParentNode = _Data[ParentIndex];

				// Determine a subtree that requires rebalancing
				const auto Dir = (SubtreeIndex == ParentNode._Child[LEFT]) ? LEFT : RIGHT;
				const auto OppositeDir = 1 - Dir;

				auto SiblingIndex = ParentNode._Child[OppositeDir];
				if (!TreeNodeIsBlack(SiblingIndex))
				{
					// Propagate red up from the sibling subtree
					_Data[SiblingIndex]._Color = BLACK;
					ParentNode._Color = RED;
					TreeRotate(ParentIndex, Dir);
					SiblingIndex = ParentNode._Child[OppositeDir];
				}

				if (SiblingIndex == INVALID_INDEX)
				{
					SubtreeIndex = ParentIndex;
				}
				else
				{
					auto pSiblingNode = &_Data[SiblingIndex];
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
							TreeRotate(SiblingIndex, OppositeDir);
							SiblingIndex = ParentNode._Child[OppositeDir];
							pSiblingNode = &_Data[SiblingIndex];
						}

						// Finish rebalancing
						pSiblingNode->_Color = ParentNode._Color;
						ParentNode._Color = BLACK;
						_Data[pSiblingNode->_Child[OppositeDir]]._Color = BLACK;
						TreeRotate(ParentIndex, Dir);
						break;
					}
				}

				ParentIndex = _Data[SubtreeIndex]._Parent;
			}

			_Data[SubtreeIndex]._Color = BLACK;
		}
	}

	TIndex PrevBusyCell(TIndex Index, TIndex& PrevFreeIndex) const noexcept
	{
		if (Index == 0) return INVALID_INDEX;
		--Index;

		while (Index == PrevFreeIndex)
		{
			if (Index == 0) return INVALID_INDEX;
			--Index;
			PrevFreeIndex = TreePrevNode(PrevFreeIndex);
		}

		return Index;
	}

	bool AllocateFreeCell(TIndex Index)
	{
		const auto InternalSize = sparse_size();
		if (Index < InternalSize)
		{
			TreeRemove(Index);
			return true;
		}

		if (Index + 1 >= MAX_CAPACITY)
		{
			n_assert2(false, "CSparseArray2 max capacity exceeded");
			return false;
		}

		_Data.resize(Index + 1);
		for (auto i = InternalSize; i < Index; ++i)
			TreeInsert(i, _LastFreeIndex, true);

		return true;
	}

	void FreeAllocatedCell(TIndex Index) noexcept
	{
		// If erasing at the tail, pop it from _Data instead of adding to the tree
		if (Index + 1 == _Data.size() && Index != _LastFreeIndex)
		{
			--_Size;
			UnsafeAt(Index).~T();
			_Data.pop_back();
			return;
		}

		// Silently skip inserting an already free node
		const auto Found = TreeFindLowerBound(Index);
		if (Found.Node == Index) return;

		--_Size;
		UnsafeAt(Index).~T();
		TreeInsert(Index, Found.Parent, Found.IsRightChild);
	}

	void CopyDataFrom(const CSparseArray2& Other)
	{
		TIndex Index = 0;
		TIndex NextFreeIndex = _FirstFreeIndex;
		for (TIndex Index = 0; Index < Other._Data.size(); ++Index)
		{
			auto& Cell = _Data[Index];
			if (Index == NextFreeIndex)
			{
				const auto& OtherCell = Other._Data[Index];
				Cell._Child[0] = OtherCell._Child[0];
				Cell._Child[1] = OtherCell._Child[1];
				Cell._Parent = OtherCell._Parent;
				Cell._Color = OtherCell._Color;

				NextFreeIndex = Other.TreeNextNode(NextFreeIndex);
			}
			else
			{
				new (Cell._ElementStorage) T(Other.UnsafeAt(Index));
			}
		}
	}

	DEM_FORCE_INLINE void MoveUnsafe(TIndex BusyIndex, TIndex FreeIndex) noexcept
	{
		T& Object = UnsafeAt(BusyIndex);
		new (_Data[FreeIndex]._ElementStorage) T(std::move(Object));
		Object.~T();
	}

	template <class... TArgs>
	bool EmplaceAtFree(TIndex FreeIndex, TArgs&&... ValueConstructionArgs)
	{
		if (!AllocateFreeCell(FreeIndex)) return false;
		++_Size;
		new (_Data[FreeIndex]._ElementStorage) T(std::forward<TArgs>(ValueConstructionArgs)...);
		return true;
	}

	T& UnsafeAt(TIndex Index) noexcept { return *reinterpret_cast<T*>(_Data[Index]._ElementStorage); }
	const T& UnsafeAt(TIndex Index) const noexcept { return *reinterpret_cast<const T*>(_Data[Index]._ElementStorage); }

public:

	CSparseArray2() = default;

	CSparseArray2(const CSparseArray2& Other)
		: _Data(Other._Data.size())
		, _FirstFreeIndex(Other._FirstFreeIndex)
		, _LastFreeIndex(Other._LastFreeIndex)
		, _TreeRootIndex(Other._TreeRootIndex)
		, _Size(Other._Size)
	{
		CopyDataFrom(Other);
	}

	CSparseArray2(CSparseArray2&& Other) noexcept
		: _Data(std::move(Other._Data))
		, _FirstFreeIndex(Other._FirstFreeIndex)
		, _LastFreeIndex(Other._LastFreeIndex)
		, _TreeRootIndex(Other._TreeRootIndex)
		, _Size(Other._Size)
	{
		Other._FirstFreeIndex = INVALID_INDEX;
		Other._LastFreeIndex = INVALID_INDEX;
		Other._TreeRootIndex = INVALID_INDEX;
		Other._Size = 0;
	}

	CSparseArray2& operator =(const CSparseArray2& Other)
	{
		for (auto it = begin(); it != end(); ++it)
			(*it).~T();

		_FirstFreeIndex = Other._FirstFreeIndex;
		_LastFreeIndex = Other._LastFreeIndex;
		_TreeRootIndex = Other._TreeRootIndex;
		_Size = Other._Size;

		_Data.resize(Other._Data.size());
		CopyDataFrom(Other);
	}

	CSparseArray2& operator =(CSparseArray2&& Other) noexcept
	{
		for (auto it = begin(); it != end(); ++it)
			(*it).~T();

		_Data = std::move(Other._Data);

		_FirstFreeIndex = Other._FirstFreeIndex;
		_LastFreeIndex = Other._LastFreeIndex;
		_TreeRootIndex = Other._TreeRootIndex;
		_Size = Other._Size;

		Other._FirstFreeIndex = INVALID_INDEX;
		Other._LastFreeIndex = INVALID_INDEX;
		Other._TreeRootIndex = INVALID_INDEX;
		Other._Size = 0;

		return *this;
	}

	void swap(CSparseArray2& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			std::swap(_Data, Other._Data);
			std::swap(_FirstFreeIndex, Other._FirstFreeIndex);
			std::swap(_LastFreeIndex, Other._LastFreeIndex);
			std::swap(_TreeRootIndex, Other._TreeRootIndex);
			std::swap(_Size, Other._Size);
		}
	}

	void reserve(TIndex NewCapacity) { _Data.reserve(NewCapacity); }

	bool is_filled(TIndex Index) const noexcept { return Index < sparse_size() && TreeFindLowerBound(Index).Node != Index; }

	TIndex first_free_index(TIndex From = 0) const noexcept
	{
		if (From <= _FirstFreeIndex) return _FirstFreeIndex;
		if (From > _LastFreeIndex) return INVALID_INDEX;
		const auto Lower = TreeFindLowerBound(From).Node;
		return (Lower < From) ? TreeNextNode(Lower) : Lower;
	}

	TIndex next_free_index(TIndex FreeIndex) const noexcept { return TreeNextNode(FreeIndex); }
	TIndex prev_free_index(TIndex FreeIndex) const noexcept { return TreePrevNode(FreeIndex); }

	iterator insert(const T& Value) { return emplace(Value); }
	iterator insert(T&& Value) { return emplace(std::move(Value)); }
	iterator insert_at_free(TIndex FreeIndex, const T& Value) { return emplace_at_free(FreeIndex, Value); }
	iterator insert_at_free(TIndex FreeIndex, T&& Value) { return emplace_at_free(FreeIndex, std::move(Value)); }

	template <class... TArgs>
	iterator emplace(TArgs&&... ValueConstructionArgs)
	{
		return emplace_at_free(_FirstFreeIndex, std::forward<TArgs>(ValueConstructionArgs)...);
	}

	// NB: this method assumes that FreeIndex is pointing to freed or inexistent cell. INVALID_INDEX is treated as push_back.
	template <class... TArgs>
	iterator emplace_at_free(TIndex FreeIndex, TArgs&&... ValueConstructionArgs)
	{
		const auto Index = (FreeIndex == INVALID_INDEX) ? sparse_size() : FreeIndex;
		return EmplaceAtFree(Index, std::forward<TArgs>(ValueConstructionArgs)...) ? iterator(Index, *this) : end();
	}

	const_iterator erase(const_iterator It) noexcept { auto It2 = It; ++It2; FreeAllocatedCell(It._CurrIndex); return It2; }
	iterator erase(iterator It) noexcept { auto It2 = It; ++It2; FreeAllocatedCell(It._CurrIndex); return It2; }
	void erase(TIndex Index) noexcept { if (Index < sparse_size()) FreeAllocatedCell(Index); }

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
		while (!_Data.empty() && _LastFreeIndex == _Data.size() - 1)
		{
			TreeRemove(_LastFreeIndex);
			_Data.pop_back();
		}

		_Data.shrink_to_fit();
	}

	// Minimizes moves
	void compact() noexcept
	{
		if (_FirstFreeIndex < _Size)
		{
			// Iterate busy cells back to front and free cells front to back, stop when overlap
			auto PrevFreeIndex = _LastFreeIndex;
			auto BusyIndex = PrevBusyCell(sparse_size(), PrevFreeIndex);
			while (PrevFreeIndex >= _FirstFreeIndex)
			{
				const auto FreeIndex = _FirstFreeIndex;
				TreeRemove(_FirstFreeIndex);
				MoveUnsafe(BusyIndex, FreeIndex);

				if (PrevFreeIndex == FreeIndex) break;

				BusyIndex = PrevBusyCell(BusyIndex, PrevFreeIndex);
			}
		}

		_Data.resize(_Size);
		_FirstFreeIndex = INVALID_INDEX;
		_LastFreeIndex = INVALID_INDEX;
		_TreeRootIndex = INVALID_INDEX;
	}

	// Preserves element ordering
	void compact_ordered() noexcept
	{
		auto InsertPos = _FirstFreeIndex;
		while (InsertPos < _Size)
		{
			// Find a range of busy cells to shift
			TIndex RangeStartIndex;
			TIndex RangeEndIndex;
			do
			{
				RangeStartIndex = _FirstFreeIndex + 1;
				TreeRemove(_FirstFreeIndex);
				RangeEndIndex = (_FirstFreeIndex == INVALID_INDEX) ? sparse_size() : _FirstFreeIndex;
			}
			while (RangeStartIndex == RangeEndIndex && _FirstFreeIndex != INVALID_INDEX);

			// Move cell data if the range is not empty
			if (RangeStartIndex < RangeEndIndex)
			{
				if constexpr (std::is_trivially_copyable_v<T>)
				{
					std::memmove(&_Data[InsertPos], &_Data[RangeStartIndex], (RangeEndIndex - RangeStartIndex) * sizeof(CCell));
					InsertPos += (RangeEndIndex - RangeStartIndex);
				}
				else
				{
					while (RangeStartIndex < RangeEndIndex)
						MoveUnsafe(RangeStartIndex++, InsertPos++);
				}
			}
		}

		_Data.resize(_Size);
		_FirstFreeIndex = INVALID_INDEX;
		_LastFreeIndex = INVALID_INDEX;
		_TreeRootIndex = INVALID_INDEX;
	}

	TIndex min_busy_index() const noexcept { return _Size ? cbegin()._CurrIndex : INVALID_INDEX; }
	TIndex max_busy_index() const noexcept { return _Size ? (--cend())._CurrIndex : INVALID_INDEX; }
	constexpr TIndex max_size() const noexcept { return MAX_CAPACITY; }
	TIndex size() const noexcept { return _Size; }
	TIndex sparse_size() const noexcept { return static_cast<TIndex>(_Data.size()); }
	TIndex capacity() const noexcept { return static_cast<TIndex>(_Data.capacity()); }
	bool empty() const noexcept { return !_Size; }

	iterator begin() noexcept { return iterator(0, *this); }
	const_iterator begin() const noexcept { return const_iterator(0, *this); }
	iterator end() noexcept { return iterator(sparse_size(), *this); }
	const_iterator end() const noexcept { return const_iterator(sparse_size(), *this); }
	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
	const_iterator cbegin() const noexcept { return begin(); }
	const_iterator cend() const noexcept { return end(); }
	const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	const_reverse_iterator crend() const noexcept { return rend(); }
	iterator iterator_at(TIndex Index) noexcept { return iterator(Index, *this); }
	const_iterator iterator_at(TIndex Index) const noexcept { return const_iterator(Index, *this); }
	const_iterator const_iterator_at(TIndex Index) const noexcept { return const_iterator(Index, *this); }

	T& at(const TIndex Index) //noexcept(std::is_nothrow_default_constructible_v<T>) - EmplaceAtFree can throw if capacity reached
	{
		if constexpr (std::is_default_constructible_v<T>)
		{
			if (!is_filled(Index))
				n_verify(EmplaceAtFree(Index));
		}
		else
		{
			n_assert(is_filled(Index));
		}

		return UnsafeAt(Index);
	}

	const T& at(const TIndex Index) const { n_assert(is_filled(Index)); return UnsafeAt(Index); }

	T& operator[](TIndex Index) noexcept { return UnsafeAt(Index); }
	const T& operator[](const TIndex Index) const noexcept { return UnsafeAt(Index); }

	bool operator ==(const CSparseArray2<T, TIndex>& Other) const
	{
		return _FirstFreeIndex == Other._FirstFreeIndex &&
			_LastFreeIndex == Other._LastFreeIndex &&
			_TreeRootIndex == Other._TreeRootIndex &&
			_Size == Other._Size &&
			_Data == Other._Data;
	}

	bool operator !=(const CSparseArray2<T, TIndex>& Other) const { return !(a == b); }
};

}
