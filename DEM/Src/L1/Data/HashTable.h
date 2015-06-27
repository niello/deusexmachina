#pragma once
#ifndef __DEM_L1_HASH_TABLE_H__
#define __DEM_L1_HASH_TABLE_H__

#include <System/System.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>
#include <Data/HashPairT.h>

// Hash table that uses sorted arrays as chains

template<class TKey, class TVal>
class CHashTable
{
public:

	typedef CHashPairT<TKey, TVal> CPair;
	typedef CArray<CPair> CChain;

protected:

	// Since internal float representation is machine-dependent, C++ doesn't allow static const float =/
	//???as per-map params?
	#define GROW_FACTOR    2.0f
	#define GROW_THRESHOLD 1.5f

	CFixedArray<CChain>	Chains;
	DWORD				Count;

	void Grow(DWORD NewCapacity);

public:

	class CIterator
	{
	private:

		CHashTable<TKey, TVal>*					pTable;
		typename CFixedArray<CChain>::CIterator	ItChain;
		typename CChain::CIterator				It;

	public:

		CIterator(CHashTable<TKey, TVal>* Tbl): pTable(Tbl)
		{
			if (pTable)
			{
				ItChain = pTable->Chains.Begin();
				while (ItChain < pTable->Chains.End() && !ItChain->GetCount()) ++ItChain;
				It = ItChain < pTable->Chains.End() ? ItChain->Begin() : NULL;
			}
			else
			{
				ItChain = NULL;
				It = NULL;
			}
		}

		const TKey&	GetKey() const { n_assert(It); return It->GetKey(); }
		TVal&		GetValue() { n_assert(It); return It->GetValue(); }
		const TVal&	GetValue() const { n_assert(It); return It->GetValue(); }
		bool		IsEnd() const { return !It; }
					operator bool() const { return !!It; }
		TVal*		operator ->() const { n_assert_dbg(It); return &It->GetValue(); }
		TVal&		operator *() const { n_assert_dbg(It); return It->GetValue(); }

		CIterator&	operator ++()
		{
			n_assert(It);
			++It;
			if (It == ItChain->End())
			{
				do ++ItChain; while (ItChain < pTable->Chains.End() && !ItChain->GetCount());
				It = ItChain < pTable->Chains.End() ? ItChain->Begin() : NULL;
			}
			return *this;
		}
	};

	static const DWORD DEFAULT_SIZE = 64;

	CHashTable(DWORD Capacity = DEFAULT_SIZE);
	CHashTable(const CHashTable<TKey, TVal>& Other): Chains(Other.Chains), Count(Other.Count) {}

	void		Add(const CPair& Pair);
	void		Add(const TKey& Key, const TVal& Value) { Add(CPair(Key, Value)); }
	bool		Remove(const TKey& Key);
	//!!!bool		Remove(const CIterator& It);
	void		Clear();
	bool		Contains(const TKey& Key) const;
	bool		Get(const TKey& Key, TVal& Value) const;
	TVal*		Get(const TKey& Key) const;
	TVal&		At(const TKey& Key) const { TVal* pVal = Get(Key); n_assert(pVal); return *pVal; } // Entry must exist
	TVal&		At(const TKey& Key); // Entry will be added
	void		CopyToArray(CChain& OutData) const;

	CIterator	Begin() { return CIterator(this); }

	int			GetCount() const { return Count; }
	int			Capacity() const { return Chains.GetCount(); }
	bool		IsEmpty() const { return !Count; }

	void		operator =(const CHashTable<TKey, TVal>& Other) { if (this != &Other) { Chains = Other.Chains; Count = Other.Count; } }
	TVal&		operator [](const TKey& Key) const { return At(Key); }
	//TVal&		operator[](const TKey& Key) { return At(Key); }
};

template<class TKey, class TVal>
inline CHashTable<TKey, TVal>::CHashTable(DWORD Capacity = DEFAULT_SIZE): Chains(Capacity), Count(0)
{
	// Since collision is more of exception, set grow size to 1 to economy memory
	for (DWORD i = 0; i < Chains.GetCount(); ++i)
		Chains[i].SetGrowSize(1);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Grow(DWORD NewCapacity)
{
	if (NewCapacity == Chains.GetCount()) return;
	CChain Tmp;
	CopyToArray(Tmp);
	Chains.SetSize(NewCapacity);
	for (int i = 0; i < Tmp.GetCount(); ++i) Add(Tmp[i]);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Add(const CPair& Pair)
{
	if (Count >= (DWORD)(Chains.GetCount() * GROW_THRESHOLD))
		Grow((DWORD)(Chains.GetCount() * GROW_FACTOR));
	CChain& Chain = Chains[Pair.GetKeyHash() % Chains.GetCount()];
	bool HasEqual;
	int Idx = Chain.FindClosestIndexSorted(Pair, &HasEqual);
	n_assert(!HasEqual);
	Chain.Insert(Idx, Pair);
	++Count;
}
//---------------------------------------------------------------------

// Returns true if element really has been erased
template<class TKey, class TVal>
bool CHashTable<TKey, TVal>::Remove(const TKey& Key)
{
	if (!Count) FAIL;
	CPair HashedKey(Key);
	CChain& Chain = Chains[HashedKey.GetKeyHash() % Chains.GetCount()];
	if (!Chain.RemoveByValueSorted(HashedKey)) FAIL;
	--Count;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Clear()
{
	for (DWORD i = 0; i < Chains.GetCount(); ++i)
		Chains[i].Clear();
	Count = 0;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
inline bool CHashTable<TKey, TVal>::Contains(const TKey& Key) const
{
	if (!Count) FAIL;
	CPair HashedKey(Key);
	return Chains[HashedKey.GetKeyHash() % Chains.GetCount()].ContainsSorted(HashedKey);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
bool CHashTable<TKey, TVal>::Get(const TKey& Key, TVal& Value) const
{
	TVal* pVal = Get(Key);
	if (!pVal) FAIL;
	Value = *pVal;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
TVal* CHashTable<TKey, TVal>::Get(const TKey& Key) const
{
	if (Count > 0)
	{
		CPair HashedKey(Key);
		CChain& Chain = Chains[HashedKey.GetKeyHash() % Chains.GetCount()];
		if (Chain.GetCount() == 1)
		{
			if (Chain[0] == HashedKey) return &Chain[0].GetValue();
		}
		else if (Chain.GetCount() > 1)
		{
			int ElmIdx = Chain.FindIndexSorted(HashedKey);
			if (ElmIdx != INVALID_INDEX) return &Chain[ElmIdx].GetValue();
		}
	}
	return NULL;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
TVal& CHashTable<TKey, TVal>::At(const TKey& Key)
{
	CPair HashedKey(Key);
	CChain& Chain = Chains[HashedKey.GetKeyHash() % Chains.GetCount()];

	int Idx = 0;
	if (Count > 0)
	{
		if (Chain.GetCount() == 1)
		{
			CPair& Elm = Chain[0];
			if (Elm == HashedKey) return Elm.GetValue();
			if (Elm < HashedKey) Idx = 1;
		}
		else if (Chain.GetCount() > 1)
		{
			bool HasEqual;
			Idx = Chain.FindClosestIndexSorted(HashedKey, &HasEqual);
			if (HasEqual) return Chain[Idx - 1].GetValue();
		}
	}

	CChain::CIterator It = Chain.Insert(Idx, HashedKey);
	++Count;

	if (Count >= (DWORD)(Chains.GetCount() * GROW_THRESHOLD))
	{
		Grow((DWORD)(Chains.GetCount() * GROW_FACTOR));
		TVal* pVal = Get(Key);
		n_assert(pVal);
		return *pVal;
	}
	else return It->GetValue();
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::CopyToArray(CChain& OutData) const
{
	if (!Count) return;
	OutData.Resize(OutData.GetCount() + Count);
	for (DWORD i = 0; i < Chains.GetCount(); ++i)
	{
		CChain& Chain = Chains[i];
		for (CChain::CIterator It = Chain.Begin(); It != Chain.End(); ++It)
			OutData.Add(*It);
	}
}
//---------------------------------------------------------------------

#endif
