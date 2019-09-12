#pragma once
#ifndef __DEM_L1_DICTIONARY_H__
#define __DEM_L1_DICTIONARY_H__

#include <Data/PairT.h>
#include <Data/Array.h>

// Associative container, internally implemented as an array of key-value pairs sorted by key

//!!!FIXME: test RemoveAt without a KeepOrder flag! must be a bug!

// Extension of CArray flags
enum
{
	Dict_InBeginAdd = 0x04	// Internal
};

#define CDict CDictionary

template<class TKey, class TValue>
class CDictionary
{
private:

	typedef CPairT<TKey, TValue> CPair;

	CArray<CPair> Pairs;

public:

	typedef typename CArray<CPair>::CIterator CIterator;

	CDictionary() {}
	CDictionary(UPTR Alloc, UPTR Grow, bool DoubleGrow = false): Pairs(Alloc, Grow) { Pairs.Flags.Set(Array_DoubleGrowSize); }
	CDictionary(const CDictionary<TKey, TValue>& Other): Pairs(Other.Pairs) {}

	void			BeginAdd() { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.Flags.Set(Dict_InBeginAdd); }
	void			BeginAdd(UPTR AddCount);
	void			EndAdd() { n_assert(Pairs.Flags.Is(Dict_InBeginAdd)); Pairs.Flags.Clear(Dict_InBeginAdd); Pairs.Sort(); }
	bool			IsInAddMode() const { return Pairs.Flags.Is(Dict_InBeginAdd); }
	TValue&			Add(const CPairT<TKey, TValue>& Pair) { return Pairs.Flags.Is(Dict_InBeginAdd) ? Pairs.Add(Pair)->GetValue() : Pairs.InsertSorted(Pair)->GetValue(); }
	TValue&			Add(const TKey& Key, const TValue& Value) { return Add(CPair(Key, Value)); }
	TValue&			Add(const TKey& Key) { return Add(CPair(Key)); }
	bool			Remove(const TKey& Key);
	void			RemoveAt(IPTR Idx) { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.RemoveAt(Idx); }
	IPTR			FindIndex(const TKey& Key) const { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); return Pairs.FindIndexSorted(Key); }
	bool			Get(const TKey& Key, TValue& Out) const;
	TValue*			Get(const TKey& Key) const;
	TValue&			GetOrAdd(const TKey& Key);
	void			Set(const TKey& Key, const TValue& Value);
	bool			Contains(const TKey& Key) const { return Pairs.FindIndexSorted(Key) != INVALID_INDEX; }
	UPTR			GetCount() const { return Pairs.GetCount(); }
	void			Clear(bool FreeMemory = false) { Pairs.Clear(FreeMemory); Pairs.Flags.Clear(Dict_InBeginAdd); }
	bool			IsEmpty() const { return !Pairs.GetCount(); }

	const TKey&		KeyAt(IPTR Idx) const { return Pairs[Idx].GetKey(); }
	TValue&			ValueAt(IPTR Idx) { return Pairs[Idx].GetValue(); }
	const TValue&	ValueAt(IPTR Idx) const { return Pairs[Idx].GetValue(); }

	CIterator		Begin() const { return Pairs.Begin(); }
	CIterator		End() const { return Pairs.End(); }

	void			Copy(CDictionary<TKey, TValue>& Out) const;
	void			CopyToArray(CArray<TValue>& Out) const;

	void			operator =(const CDictionary<TKey, TValue>& Other) { Pairs = Other.Pairs; }
	TValue&			operator [](const TKey& Key) { IPTR Idx = FindIndex(Key); n_assert(Idx != INVALID_INDEX); return Pairs[Idx].GetValue(); }
	const TValue&	operator [](const TKey& Key) const { IPTR Idx = FindIndex(Key); n_assert(Idx != INVALID_INDEX); return Pairs[Idx].GetValue(); }
};

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::BeginAdd(UPTR AddCount)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	Pairs.Resize(Pairs.GetCount() + AddCount);
	Pairs.Flags.Set(Dict_InBeginAdd);
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
bool CDictionary<TKey, TValue>::Remove(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	IPTR Idx = Pairs.FindIndexSorted(Key);
	if (Idx == INVALID_INDEX) return false;
	RemoveAt(Idx);
	return true;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
bool CDictionary<TKey, TValue>::Get(const TKey& Key, TValue& Out) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	IPTR Idx = Pairs.FindIndexSorted(Key);
	if (Idx == INVALID_INDEX) FAIL;
	Out = Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue* CDictionary<TKey, TValue>::Get(const TKey& Key) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	IPTR Idx = Pairs.FindIndexSorted(Key);
	return (Idx == INVALID_INDEX) ? nullptr : &Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue& CDictionary<TKey, TValue>::GetOrAdd(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	IPTR Idx = Pairs.FindIndexSorted(Key);
	return (Idx == INVALID_INDEX) ? Add(Key) : Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::Set(const TKey& Key, const TValue& Value)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	IPTR Idx = Pairs.FindIndexSorted(Key);
	if (Idx == INVALID_INDEX) Add(Key, Value);
	else ValueAt(Idx) = Value;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::Copy(CDictionary<TKey, TValue>& Out) const
{
	Out.Pairs.Copy(Pairs);
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::CopyToArray(CArray<TValue>& Out) const
{
	for (UPTR i = 0; i < Pairs.GetCount(); ++i)
		Out.Add(Pairs[i].GetValue());
}
//---------------------------------------------------------------------

#endif