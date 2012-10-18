#pragma once
#ifndef __DEM_L1_HASH_TABLE_H__
#define __DEM_L1_HASH_TABLE_H__

#include <StdDEM.h>
#include <util/nfixedarray.h>
#include <util/narray.h>
#include <util/nkeyvaluepair.h>
#include <util/Hash.h>

// Hash table that uses sorted arrays as chains

//???write Grow()?

//namespace Util
//{

template<class TKey, class TVal>
class HashTable
{
protected:

	typedef nKeyValuePair<TKey, TVal> CPair;
	typedef nArray<CPair> CChain;

	nFixedArray<CChain>	Chains;
	int					Count;

public:

	HashTable(): Chains(128), Count(0) {}
    HashTable(int Capacity): Chains(Capacity), Count(0) {}
	HashTable(const HashTable<TKey, TVal>& Other): Chains(Other.Chains), Count(Other.Count) {}

    void	Add(const nKeyValuePair<TKey, TVal>& Pair);
	void	Add(const TKey& Key, const TVal& value) { Add(CPair(Key, value)); }
    bool	Erase(const TKey& Key);
	void	Clear();
    bool	Contains(const TKey& Key) const;
	bool	Get(const TKey& Key, TVal& Value) const;
	TVal*	Get(const TKey& Key) const;
    void	CopyToArray(nArray<nKeyValuePair<TKey, TVal>>& OutData) const;

	int		Size() const { return Count; }
	int		Capacity() const { return Chains.Size(); }
	bool	IsEmpty() const { return !Count; }

	void	operator =(const HashTable<TKey, TVal>& Other) { if (this != &Other) { Chains = Other.Chains; Count = Other.Count; } }
	TVal&	operator [](const TKey& Key) const { TVal* pVal = Get(Key); n_assert(pVal); return *pVal; }
};

template<class TKey, class TVal>
void HashTable<TKey, TVal>::Add(const nKeyValuePair<TKey, TVal>& Pair)
{
	CChain& Chain = Chains[Hash(Pair.Key()) % Chains.Size()];
	n_assert(!Count || Chain.BinarySearchIndex(Pair.Key()) == INVALID_INDEX);
	Chain.InsertSorted(Pair);
	++Count;
}
//---------------------------------------------------------------------

// Returns true if element war really erased
template<class TKey, class TVal>
bool HashTable<TKey, TVal>::Erase(const TKey& Key)
{
	if (!Count) FAIL;
	CChain& Chain = Chains[Hash(Key) % Chains.Size()];
	int ElmIdx = Chain.BinarySearchIndex(Key);
	if (ElmIdx == INVALID_INDEX) FAIL;
	Chain.Erase(ElmIdx);
	--Count;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void HashTable<TKey, TVal>::Clear()
{
	for (int i = 0; i < Chains.Size(); i++)
		Chains[i].Clear();
	Count = 0;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
inline bool HashTable<TKey, TVal>::Contains(const TKey& Key) const
{
	return Count && (Chains[Hash(Key) % Chains.Size()].BinarySearchIndex(Key) != INVALID_INDEX);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
bool HashTable<TKey, TVal>::Get(const TKey& Key, TVal& Value) const
{
	TVal* pVal = Get(Key);
	if (!pVal) FAIL;
	Value = *pVal;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
TVal* HashTable<TKey, TVal>::Get(const TKey& Key) const
{
	if (Count > 0)
	{
		CChain& Chain = Chains[Hash(Key) % Chains.Size()];
		if (Chain.Size() == 1)
		{
			if (Chain[0].Key() == Key) return &Chain[0].Value();
		}
		else if (Chain.Size() > 1)
		{
			int ElmIdx = Chain.BinarySearchIndex(Key);
			if (ElmIdx != INVALID_INDEX) return &Chain[ElmIdx].Value();
		}
	}
	return NULL;
}
//---------------------------------------------------------------------

//!!!
// Too slow. Need iterator.
template<class TKey, class TVal>
void HashTable<TKey, TVal>::CopyToArray(nArray<nKeyValuePair<TKey, TVal>>& OutData) const
{
	for (int i = 0; i < Chains.Size(); i++)
		OutData.AppendArray(Chains[i]);
}
//---------------------------------------------------------------------

//}

#endif
