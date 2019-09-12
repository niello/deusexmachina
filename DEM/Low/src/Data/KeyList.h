#pragma once
#ifndef __DEM_L1_LINKED_LIST_SET_H__
#define __DEM_L1_LINKED_LIST_SET_H__

#include <Data/Dictionary.h>
#include <Data/List.h>

// KeyList is an array of linked lists, sorted by a curtain key attribute.
// Elements are added to one of lists based on their key.
// NB: if key value of already inserted element changes, element placing must be adjusted manually by reinserting!

// Class TObject must implement following interface:
// - TKey GetKey() const;

namespace Data
{

template<class TKey, class TObject>
class CKeyList
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
	//UPTR							TotalCount;

public:

	typedef typename CList<TObject>::CIterator CIterator;

	~CKeyList();

	CIterator	Add(const TObject& Object);
	void		Remove(CIterator It, TObject* pOutValue = nullptr);
	bool		RemoveByValue(const TObject& Object);

	CIterator	Find(const TObject& Object) const;
	CIterator	End() const { return CIterator(nullptr); }
	TKey		GetKeyAt(IPTR Idx) const { return Lists.KeyAt(Idx); }
	CIterator	GetHead(TKey Key) const;
	CIterator	GetHeadAt(IPTR Idx) const { return Lists.ValueAt(Idx)->Begin(); }
	UPTR		GetListCount() const { return (UPTR)Lists.GetCount(); }
	UPTR		GetCount(TKey Key) const;
	//UPTR		GetTotalCount() const { return TotalCount; }
};
//---------------------------------------------------------------------

template<class TKey, class TObject>
CKeyList<TKey,TObject>::~CKeyList()
{
	for (UPTR i = 0; i < Lists.GetCount(); ++i)
		n_delete(Lists.ValueAt(i));
	Lists.Clear();
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
typename CKeyList<TKey, TObject>::CIterator CKeyList<TKey,TObject>::Add(const TObject& Object)
{
	CList<TObject>* pList;

	TKey Key = ObjTraits<TObject>::GetKey(Object);
	IPTR Idx = Lists.FindIndex(Key);
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
inline void CKeyList<TKey, TObject>::Remove(typename CKeyList<TKey, TObject>::CIterator It, TObject* pOutValue)
{	
	IPTR Idx = Lists.FindIndex((*It)->GetKey());
	if (Idx != INVALID_INDEX) Lists.ValueAt(Idx)->Remove(It, pOutValue);
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
bool CKeyList<TKey, TObject>::RemoveByValue(const TObject& Object)
{	
	TKey Key = ObjTraits<TObject>::GetKey(Object);
	IPTR Idx = Lists.FindIndex(Key);
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
typename CKeyList<TKey, TObject>::CIterator CKeyList<TKey, TObject>::Find(const TObject& Object) const
{
	TKey Key = ObjTraits<TObject>::GetKey(Object);
	IPTR Idx = Lists.FindIndex(Key);
	if (Idx == INVALID_INDEX) return nullptr;
	CList<TObject>* pList = Lists.ValueAt(Idx);
	return pList->Find(Object, pList->Begin());
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline typename CKeyList<TKey, TObject>::CIterator CKeyList<TKey, TObject>::GetHead(TKey Key) const
{	
	IPTR Idx = Lists.FindIndex(Key);
	return (Idx != INVALID_INDEX) ? Lists.ValueAt(Idx)->Begin() : nullptr;
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline UPTR CKeyList<TKey, TObject>::GetCount(TKey Key) const
{	
	IPTR Idx = Lists.FindIndex(Key);
	return (Idx != INVALID_INDEX) ? Lists.ValueAt(Idx)->GetCount() : 0;
}
//---------------------------------------------------------------------

};

#endif
