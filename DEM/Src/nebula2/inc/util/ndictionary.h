#ifndef N_DICTIONARY_H
#define N_DICTIONARY_H
//------------------------------------------------------------------------------
/**
    @class nDictionary

    A collection of Key/Value pairs with quick Value retrieval
    by Key at roughly O(log n). Insertion is O(n), or faster
    if the BeginAdd()/EndAdd() methods are used.
    Find-by-Key behavior is undefined if several identical
    keys are added to the array.

    Internally the dictionary is implemented as a sorted array.

    On insertion performance:
    Key/Value pairs can be added at any time with the Add() methods.
    This uses Array::InsertSorted() which is roughly O(n). If keys
    are added between BeginAdd()/EndAdd() the keys will be added
    unsorted, and only one Array::Sort() happens in End(). This may
    be faster depending on how many keys are added to the dictionary,
    and how big the dictionary is.

    This class has been backported from the Nebula3 Core Layer.

    (C) 2006 Radon Labs GmbH
*/
#include "util/narray.h"
#include "util/PairT.h"

template<class TKey, class TValue>
class nDictionary
{
private:

	typedef CPairT<TKey, TValue> CPair;

	nArray<CPair>	Pairs;
	bool			IsInBeginAdd;

public:

	nDictionary(): IsInBeginAdd(false) {}
	nDictionary(int Alloc, int Grow, bool DoubleGrow): Pairs(Alloc, Grow), IsInBeginAdd(false) { Pairs.SetFlags(nArray<CPair>::DoubleGrowSize); }
	nDictionary(const nDictionary<TKey, TValue>& Other): Pairs(Other.Pairs), IsInBeginAdd(Other.IsInBeginAdd) {}

	void			BeginAdd() { n_assert(!IsInBeginAdd); IsInBeginAdd = true; }
	void			BeginAdd(int num);
	void			EndAdd() { n_assert(IsInBeginAdd); IsInBeginAdd = false; Pairs.Sort(); }
	TValue&			Add(const CPairT<TKey, TValue>& Pair) { return IsInBeginAdd ? Pairs.Append(Pair).GetValue() : Pairs.InsertSorted(Pair).GetValue(); }
	TValue&			Add(const TKey& Key, const TValue& Value) { return Add(CPair(Key, Value)); }
	TValue&			Add(const TKey& Key) { return Add(CPair(Key)); }
	bool			Erase(const TKey& Key);
	void			EraseAt(int Idx) { n_assert(!IsInBeginAdd); Pairs.Erase(Idx); }
	int				FindIndex(const TKey& Key) const { n_assert(!IsInBeginAdd); return Pairs.BinarySearchIndex(Key); }
	TValue*			Get(const TKey& Key) const;
	TValue&			GetOrAdd(const TKey& Key);
	bool			Contains(const TKey& Key) const { return Pairs.BinarySearchIndex(Key) != -1; }
	int				Size() const { return Pairs.Size(); }
	void			Clear() { Pairs.Clear(); IsInBeginAdd = false; }
	bool			IsEmpty() const { return !Pairs.Size(); }

	const TKey&		KeyAtIndex(int Idx) const { return Pairs[Idx].GetKey(); }
	TValue&			ValueAtIndex(int Idx) { return Pairs[Idx].GetValue(); }
	const TValue&	ValueAtIndex(int Idx) const { return Pairs[Idx].GetValue(); }
	CPair&			PairAtIndex(int Idx) { return Pairs[Idx]; }

	void			CopyToArray(nArray<TValue>& Out) const;

	void			operator =(const nDictionary<TKey, TValue>& Other) { Pairs = Other.Pairs; IsInBeginAdd = Other.IsInBeginAdd; }
	TValue&			operator [](const TKey& Key) { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
	const TValue&	operator [](const TKey& Key) const { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
};

template<class TKey, class TValue>
void nDictionary<TKey, TValue>::BeginAdd(int num)
{
	n_assert(!IsInBeginAdd);
	Pairs.Resize(Pairs.Size() + num);
	IsInBeginAdd = true;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
bool nDictionary<TKey, TValue>::Erase(const TKey& Key)
{
	n_assert(!IsInBeginAdd);
	int Idx = Pairs.BinarySearchIndex(Key);
	if (Idx == -1) return false;
	EraseAt(Idx);
	return true;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue* nDictionary<TKey, TValue>::Get(const TKey& Key) const
{
	n_assert(!IsInBeginAdd);
	int Idx = Pairs.BinarySearchIndex(Key);
	return (Idx == -1) ? NULL : &Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue& nDictionary<TKey, TValue>::GetOrAdd(const TKey& Key)
{
	n_assert(!IsInBeginAdd);
	int Idx = Pairs.BinarySearchIndex(Key);
	return (Idx == -1) ? Add(Key) : Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void nDictionary<TKey, TValue>::CopyToArray(nArray<TValue>& Out) const
{
	for (int i = 0; i < Pairs.Size(); i++)
		Out.Append(Pairs[i].GetValue());
}
//---------------------------------------------------------------------

#endif