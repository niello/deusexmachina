#ifndef N_DICTIONARY_H
#define N_DICTIONARY_H

#include "util/narray.h"
#include "util/PairT.h"

// Dictionary is an associative array, internally implemented as array of key-value pairs sorted by key.

// Extension of nArray flags
enum
{
	Dict_InBeginAdd = 0x04	// Internal
};

template<class TKey, class TValue>
class nDictionary
{
private:

	typedef CPairT<TKey, TValue> CPair;

	nArray<CPair> Pairs;

public:

	nDictionary() {}
	nDictionary(int Alloc, int Grow, bool DoubleGrow): Pairs(Alloc, Grow) { Pairs.Flags.Set(Array_DoubleGrowSize); }
	nDictionary(const nDictionary<TKey, TValue>& Other): Pairs(Other.Pairs) {}

	void			BeginAdd() { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.Flags.Set(Dict_InBeginAdd); }
	void			BeginAdd(int num);
	void			EndAdd() { n_assert(Pairs.Flags.Is(Dict_InBeginAdd)); Pairs.Flags.Clear(Dict_InBeginAdd); Pairs.Sort(); }
	TValue&			Add(const CPairT<TKey, TValue>& Pair) { return Pairs.Flags.Is(Dict_InBeginAdd) ? Pairs.Append(Pair).GetValue() : Pairs.InsertSorted(Pair).GetValue(); }
	TValue&			Add(const TKey& Key, const TValue& Value) { return Add(CPair(Key, Value)); }
	TValue&			Add(const TKey& Key) { return Add(CPair(Key)); }
	bool			Erase(const TKey& Key);
	void			EraseAt(int Idx) { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); Pairs.EraseAt(Idx); }
	int				FindIndex(const TKey& Key) const { n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd)); return Pairs.BinarySearchIndex(Key); }
	bool			Get(const TKey& Key, TValue& Out) const;
	TValue*			Get(const TKey& Key) const;
	TValue&			GetOrAdd(const TKey& Key);
	void			Set(const TKey& Key, const TValue& Value);
	bool			Contains(const TKey& Key) const { return Pairs.BinarySearchIndex(Key) != -1; }
	int				GetCount() const { return Pairs.GetCount(); }
	void			Clear() { Pairs.Clear(); Pairs.Flags.Clear(Dict_InBeginAdd); }
	bool			IsEmpty() const { return !Pairs.GetCount(); }

	const TKey&		KeyAt(int Idx) const { return Pairs[Idx].GetKey(); }
	TValue&			ValueAt(int Idx) { return Pairs[Idx].GetValue(); }
	const TValue&	ValueAt(int Idx) const { return Pairs[Idx].GetValue(); }

	void			CopyToArray(nArray<TValue>& Out) const;

	void			operator =(const nDictionary<TKey, TValue>& Other) { Pairs = Other.Pairs; }
	TValue&			operator [](const TKey& Key) { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
	const TValue&	operator [](const TKey& Key) const { int Idx = FindIndex(Key); n_assert(Idx != -1); return Pairs[Idx].GetValue(); }
};

template<class TKey, class TValue>
void nDictionary<TKey, TValue>::BeginAdd(int num)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	Pairs.Resize(Pairs.GetCount() + num);
	Pairs.Flags.Set(Dict_InBeginAdd);
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
bool nDictionary<TKey, TValue>::Erase(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.BinarySearchIndex(Key);
	if (Idx == -1) return false;
	EraseAt(Idx);
	return true;
}
//---------------------------------------------------------------------

//???TValue*& Out to avoid copying?
template<class TKey, class TValue>
bool nDictionary<TKey, TValue>::Get(const TKey& Key, TValue& Out) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.BinarySearchIndex(Key);
	if (Idx == -1) FAIL;
	Out = Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue* nDictionary<TKey, TValue>::Get(const TKey& Key) const
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.BinarySearchIndex(Key);
	return (Idx == -1) ? NULL : &Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
TValue& nDictionary<TKey, TValue>::GetOrAdd(const TKey& Key)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.BinarySearchIndex(Key);
	return (Idx == -1) ? Add(Key) : Pairs[Idx].GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void nDictionary<TKey, TValue>::Set(const TKey& Key, const TValue& Value)
{
	n_assert(Pairs.Flags.IsNot(Dict_InBeginAdd));
	int Idx = Pairs.BinarySearchIndex(Key);
	if (Idx == -1) Add(Key, Value);
	else ValueAt(Idx) = Value;
}
//---------------------------------------------------------------------

template<class TKey, class TValue>
void nDictionary<TKey, TValue>::CopyToArray(nArray<TValue>& Out) const
{
	for (int i = 0; i < Pairs.GetCount(); i++)
		Out.Append(Pairs[i].GetValue());
}
//---------------------------------------------------------------------

#endif