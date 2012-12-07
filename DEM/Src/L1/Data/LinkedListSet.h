#pragma once
#ifndef __DEM_L1_LINKED_LIST_SET_H__
#define __DEM_L1_LINKED_LIST_SET_H__

#include <util/ndictionary.h>
#include <util/nobjectlist.h>

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
		//???or just redefine member access operator?
	};

	template <class T>
	struct ObjTraits<T*>
	{
		static TKey GetKey(const T* Object) { return Object->GetKey(); }
		//???or just redefine member access operator?
	};

	template <class T, template <class> class SP>
	struct ObjTraits<SP<T>>
	{
		static TKey GetKey(const T* Object) { return Object->GetKey(); }
		//???or just redefine member access operator?
	};

	nDictionary<TKey, nObjectList<TObject>*>	Lists;
	//int										TotalCount;

public:

	typedef nObjectNode<TObject> CElement;

	~CLinkedListSet();

	CElement*	Add(const TObject& Object);
	bool		RemoveByValue(const TObject& Object);
	void		RemoveElement(CElement* pElement);

	int			GetListCount() const { return Lists.Size(); }
	TKey		GetKeyAt(int Idx) const { return Lists.KeyAtIndex(Idx); }
	CElement*	GetHead(TKey Key) const;
	CElement*	GetHeadAt(int Idx) const { return Lists.ValueAtIndex(Idx)->GetHead(); }

	//int		GetTotalCount() const { return TotalCount; }
};
//---------------------------------------------------------------------

template<class TKey, class TObject>
CLinkedListSet<TKey,TObject>::~CLinkedListSet()
{
	for (int i = 0; i < Lists.Size(); ++i)
	{
		nObjectList<TObject>* pList = Lists.ValueAtIndex(i);
		CElement* pNode;
		while (pNode = pList->RemHead()) n_delete(pNode);
		n_delete(pList);
	}
	Lists.Clear();
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
typename CLinkedListSet<TKey, TObject>::CElement* CLinkedListSet<TKey,TObject>::Add(const TObject& Object)
{
	nObjectList<TObject>* pList;
	
	TKey Key = ObjTraits<TObject>::GetKey(Object);
	int Idx = Lists.FindIndex(Key);
	if (Idx == INVALID_INDEX)
	{
		pList = n_new(nObjectList<TObject>);
		Lists.Add(Key, pList);
	}
	else pList = Lists.ValueAtIndex(Idx);

	CElement* pNode = n_new(CElement)(Object);
	pList->AddTail(pNode);
	return pNode;
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
bool CLinkedListSet<TKey, TObject>::RemoveByValue(const TObject& Object)
{	
	TKey Key = Object->GetKey();
	int Idx = Lists.FindIndex(Key);
	if (Idx != INVALID_INDEX)
	{
		for (CElement* pCurr = Lists.ValueAtIndex(Idx)->GetHead(); pCurr; pCurr = pCurr->GetSucc())
			if (pCurr->Object == Object)
			{
				pCurr->Remove();
				n_delete(pCurr);
				OK;
			}
	}

	FAIL;
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline void CLinkedListSet<TKey, TObject>::RemoveElement(typename CLinkedListSet<TKey, TObject>::CElement* pElement)
{	
	pElement->Remove();
	n_delete(pElement);
}
//---------------------------------------------------------------------

template<class TKey, class TObject>
inline typename CLinkedListSet<TKey, TObject>::CElement* CLinkedListSet<TKey, TObject>::GetHead(TKey Key) const
{	
	int Idx = Lists.FindIndex(Key);
	return (Idx != INVALID_INDEX) ? Lists.ValueAtIndex(Idx)->GetHead() : NULL;
}
//---------------------------------------------------------------------

};

#endif
