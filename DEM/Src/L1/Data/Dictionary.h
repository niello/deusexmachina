#pragma once
#ifndef __DEM_L1_DICTIONARY_H__
#define __DEM_L1_DICTIONARY_H__

#include <Data/PairT.h>
#include <Data/Array.h>

// Associative container, internally implemented as an array of key-value pairs sorted by key

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

	CDictionary() {}
	CDictionary(int Alloc, int Grow, bool DoubleGrow): Pairs(Alloc, Grow) { Pairs.Flags.Set(Array_DoubleGrowSize); }
	CDictionary(const CDictionary<TKey, TValue>& Other): Pairs(Other.Pairs) {}

	void			BeginAdd() { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.Flags.Set(Dict_InBeginAdd); }
	void			BeginAdd(int num);
	void			EndAdd() { n_assert(Pairs.Flags.Is(Dict_InBeginAdd)); Pairs.Flags.Clear(Dict_InBeginAdd); Pairs.Sort(); }
	TValue&			Add(const CPairT<TKey, TValue>& Pair) { return Pairs.Flags.Is(Dict_InBeginAdd) ? Pairs.Add(Pair)->GetValue() : Pairs.InsertSorted(Pair)->GetValue(); }
	TValue&			Add(const TKey& Key, const TValue& Value) { return Add(CPair(Key, Value)); }
	TValue&			Add(const TKey& Key) { return Add(CPair(Key)); }
	bool			Remove(const TKey& Key);
	void			RemoveAt(int Idx) { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.RemoveAt(Idx); }
	int				FindIndex(const TKey& Key) const { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); return Pairs.FindIndexSorted(Key); }
	bool			Get(const TKey& Key, TValue& Out) const;
	TValue*			Get(const TKey& Key) const;
	TValue&			GetOrAdd(const TKey& Key);
	void			Set(const TKey& Key, const TValue& Value);
	bool			Contains(const TKey& Key) const { return Pairs.FindIndexSorted(Key) != -1; }
	int				GetCount() const { return Pairs.GetCount(); }
	void			Clear() { Pairs.Clear(); Pairs.Flags.Clear(Dict_InBeginAdd); }
	bool			IsEmpty() const { return !Pairs.GetCount(); }

	const TKey&		KeyAt(int Idx) const { return Pairs[Idx].GetKey(); }
	TValue&			ValueAt(int Idx) { return Pairs[Idx].GetValue(); }
	const TValue&	ValueAt(int Idx) const { return Pairs[Idx].GetValue(); }

	void			CopyToArray(CArray<TValue>& Out) const;

	void			operator =(const CDictionary<TKey, TValue>& Other) { Pairs = Other.Pairs; }
	TValue&			operator [](const TKey& Key) { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
	const TValue&	operator [](const TKey& Key) const { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
};

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::BeginAdd(int num)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	Pairs.Resize(Pairs.GetCount() + num);
	Pairs.Flags.Set(Dict_InBeginAdd);
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
bool CDictionary<TKey, TValue>::Remove(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.FindIndexSorted(Key);
	if (Idx == -1) return false;
	RemoveAt(Idx);
	return true;
}
//---------------------------------------------------------------------

//???TValue*& Out to avoid copying?
template<class TKey, class TValue>
bool CDictionary<TKey, TValue>::Get(const TKey& Key, TValue& Out) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.FindIndexSorted(Key);
	if (Idx == -1) FAIL;
	Out = Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue* CDictionary<TKey, TValue>::Get(const TKey& Key) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.FindIndexSorted(Key);
	return (Idx == -1) ? NULL : &Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue& CDictionary<TKey, TValue>::GetOrAdd(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.FindIndexSorted(Key);
	return (Idx == -1) ? Add(Key) : Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::Set(const TKey& Key, const TValue& Value)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.FindIndexSorted(Key);
	if (Idx == -1) Add(Key, Value);
	else ValueAt(Idx) = Value;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void CDictionary<TKey, TValue>::CopyToArray(CArray<TValue>& Out) const
{
	for (int i = 0; i < Pairs.GetCount(); i++)
		Out.Add(Pairs[i].GetValue());
}
//---------------------------------------------------------------------

#endif