#pragma once
#ifndef __DEM_L1_LINKED_LIST_SET_H__
#define __DEM_L1_LINKED_LIST_SET_H__

#include <Data/Dictionary.h>
#include <Data/List.h>

// Linked list set is a set of linked lists, grouped by a curtain key attribute.
// Elements are added to one of lists based on the value of this key.
// NB: if key value changes dynamically, element placing will not change automatically!

// Class TObject must implement following interface:
// - TKey GetKey() const;

namespace Data
{

template<class TKey, class TObject>
class CLinkedListSet
{
protected:

	template <class T>
	struct ObjTraits
	{
		static TKey GetKey(const T& Object) { return Object.GetKey(); }
	};

	template <class T>
	struct ObjTraits<T*>
	{
		static TKey GetKey(const T* Object) { return Object->GetKey(); }
	};

	template <class T, template <class> class SP>
	struct ObjTraits<SP<T>>
	{
		static TKey GetKey(const T* Object) { return Object->GetKey(); }
	};

	CDict<TKey, CList<TObject>*>	Lists;
	//int							TotalCount;

public:

	typedef typename CList<TObject>::CIterator CIterator;

	~CLinkedListSet();

	CIterator	Add(const TObject& Object);
	void		Remove(CIterator It, TObject* pOutValue = NULL);
	bool		RemoveByValue(const TObject& Object);

	TKey		GetKeyAt(int Idx) const { return Lists.KeyAt(Idx); }
	CIterator	GetHead(TKey Key) const;
	CIterator	GetHeadAt(int Idx) const { return Lists.ValueAt(Idx)->Begin(); }
	DWORD		GetListCount() const { return (DWORD)Lists.GetCount(); }
	DWORD		GetCount(TKey Key) const;
	//DWORD		GetTotalCount() const { return TotalCount; }
};
//---------------------------------------------------------------------

template<class TKey, class TObject>
CLinkedListSet<TKey,TObject>::~CLinkedListSet()
{
	for (int i = 0; i < Lists.GetCount(); ++i)
		n_delete(Lists.ValueAt(i));
	Lists.Clear();
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
typename CLinkedListSet<TKey, TObject>::CIterator CLinkedListSet<TKey,TObject>::Add(const TObject& Object)
{
	CList<TObject>* pList;

	TKey Key = ObjTraits<TObject>::GetKey(Object);
	int Idx = Lists.FindIndex(Key);
	if (Idx == INVALID_INDEX)
	{
		pList = n_new(CList<TObject>);
		Lists.Add(Key, pList);
	}
	else pList = Lists.ValueAt(Idx);

	return pList->AddBack(Object);
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline void CLinkedListSet<TKey, TObject>::Remove(typename CLinkedListSet<TKey, TObject>::CIterator It, TObject* pOutValue)
{	
	int Idx = Lists.FindIndex((*It)->GetKey());
	if (Idx != INVALID_INDEX) Lists.ValueAt(Idx)->Remove(It, pOutValue);
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
bool CLinkedListSet<TKey, TObject>::RemoveByValue(const TObject& Object)
{	
	TKey Key = Object->GetKey();
	int Idx = Lists.FindIndex(Key);
	if (Idx != INVALID_INDEX)
	{
		CList<TObject>* pList = Lists.ValueAt(Idx);
		CList<TObject>::CIterator It = pList->Find(Object, pList->Begin());
		if (It)
		{
			pList->Remove(It);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline typename CLinkedListSet<TKey, TObject>::CIterator CLinkedListSet<TKey, TObject>::GetHead(TKey Key) const
{	
	int Idx = Lists.FindIndex(Key);
	return (Idx != INVALID_INDEX) ? Lists.ValueAt(Idx)->Begin() : NULL;
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline DWORD CLinkedListSet<TKey, TObject>::GetCount(TKey Key) const
{	
	int Idx = Lists.FindIndex(Key);
	return (Idx != INVALID_INDEX) ? Lists.ValueAt(Idx)->GetCount() : 0;
}
//---------------------------------------------------------------------

};

#endif
