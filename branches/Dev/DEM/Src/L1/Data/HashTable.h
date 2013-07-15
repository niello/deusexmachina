#pragma once
#ifndef __DEM_L1_HASH_TABLE_H__
#define __DEM_L1_HASH_TABLE_H__

#include <StdDEM.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>
#include <Data/PairT.h>
#include <Data/Hash.h>

// Hash table that uses sorted arrays as chains

//???write Grow()?

//???use btHashTable from Bullet? or improve this table?

template<class TKey, class TVal>
class CHashTable
{
protected:

	typedef CPairT<TKey, TVal> CPair;
	typedef CArray<CPair> CChain; //???use dictionaries?

	CFixedArray<CChain>	Chains;
	int					Count;

public:

	class CIterator
	{
	private:

		CHashTable<TKey, TVal>*		pTable;
		DWORD						ChainIdx;
		typename CChain::CIterator	It;

	public:

		CIterator(CHashTable<TKey, TVal>* Tbl): pTable(Tbl), ChainIdx(0)
		{
			if (Tbl)
			{
				while (ChainIdx < pTable->Chains.GetCount() && !pTable->Chains[ChainIdx].GetCount()) ++ChainIdx;
				It = ChainIdx < pTable->Chains.GetCount() ? pTable->Chains[ChainIdx].Begin() : NULL;
			}
			else It = NULL;
		}

		const TKey&	GetKey() const { n_assert(It); return It->GetKey(); }
		const TVal&	GetValue() const { n_assert(It); return It->GetValue(); }
		bool		IsEnd() const { return !It; }
					operator bool() const { return !!It; }
		TVal*		operator ->() const { n_assert_dbg(It); return &It->GetValue(); }
		TVal&		operator *() const { n_assert_dbg(It); return It->GetValue(); }

		CIterator&	operator ++()
		{
			n_assert(It);
			++It;
			if (It == pTable->Chains[ChainIdx].End())
			{
				do ++ChainIdx; while (ChainIdx < pTable->Chains.GetCount() && !pTable->Chains[ChainIdx].GetCount());
				It = ChainIdx < pTable->Chains.GetCount() ? pTable->Chains[ChainIdx].Begin() : NULL;
			}
			return *this;
		}
	};

	static const int DEFAULT_SIZE = 64;

    CHashTable(int Capacity = DEFAULT_SIZE);
	CHashTable(const CHashTable<TKey, TVal>& Other): Chains(Other.Chains), Count(Other.Count) {}

    void		Add(const CPairT<TKey, TVal>& Pair);
	void		Add(const TKey& Key, const TVal& value) { Add(CPair(Key, value)); }
    bool		Remove(const TKey& Key);
	void		Clear();
    bool		Contains(const TKey& Key) const;
	bool		Get(const TKey& Key, TVal& Value) const;
	TVal*		Get(const TKey& Key) const;
    void		CopyToArray(CArray<CPairT<TKey, TVal>>& OutData) const;

	CIterator	Begin() { return CIterator(this); }

	int			GetCount() const { return Count; }
	int			Capacity() const { return Chains.GetCount(); }
	bool		IsEmpty() const { return !Count; }

	void		operator =(const CHashTable<TKey, TVal>& Other) { if (this != &Other) { Chains = Other.Chains; Count = Other.Count; } }
	TVal&		operator [](const TKey& Key) const { TVal* pVal = Get(Key); n_assert(pVal); return *pVal; }
};

template<class TKey, class TVal>
inline CHashTable<TKey, TVal>::CHashTable(int Capacity = DEFAULT_SIZE): Chains(Capacity), Count(0)
{
	// Since collision is more of exception, set grow size to 1 to economy memory
	for (DWORD i = 0; i < Chains.GetCount(); ++i)
		Chains[i].SetGrowSize(1);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Add(const CPairT<TKey, TVal>& Pair)
{
	CChain& Chain = Chains[Hash(Pair.GetKey()) % Chains.GetCount()];
	n_assert(!Count || Chain.FindIndexSorted(Pair.GetKey()) == INVALID_INDEX);
	Chain.InsertSorted(Pair);
	++Count;
}
//---------------------------------------------------------------------

// Returns true if element really has been erased
template<class TKey, class TVal>
bool CHashTable<TKey, TVal>::Remove(const TKey& Key)
{
	if (!Count) FAIL;
	CChain& Chain = Chains[Hash(Key) % Chains.GetCount()];
	int ElmIdx = Chain.FindIndexSorted(Key);
	if (ElmIdx == INVALID_INDEX) FAIL;
	Chain.RemoveAt(ElmIdx);
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
	return Count && (Chains[Hash(Key) % Chains.GetCount()].FindIndexSorted(Key) != INVALID_INDEX);
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
		CChain& Chain = Chains[Hash(Key) % Chains.GetCount()];
		if (Chain.GetCount() == 1)
		{
			if (Chain[0].GetKey() == Key) return &Chain[0].GetValue();
		}
		else if (Chain.GetCount() > 1)
		{
			int ElmIdx = Chain.FindIndexSorted(Key);
			if (ElmIdx != INVALID_INDEX) return &Chain[ElmIdx].GetValue();
		}
	}
	return NULL;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::CopyToArray(CArray<CPairT<TKey, TVal>>& OutData) const
{
	for (int i = 0; i < Chains.GetCount(); i++)
		OutData.AddArray(Chains[i]);
}
//---------------------------------------------------------------------

#endif
