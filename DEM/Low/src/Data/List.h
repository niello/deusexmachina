#pragma once
#ifndef __DEM_L1_LIST_H__
#define __DEM_L1_LIST_H__

#include <StdDEM.h>

// Implements a doubly linked list. Since list elements can be all over the
// place in memory, dynamic arrays are often the better choice, unless 
// insert/remove performance is more important then traversal performance.

namespace Data
{

template<class T>
class CList
{
private:

	class CNode
	{
		friend class CList;
		friend class CIterator;

		CNode*	pNext;
		CNode*	pPrev;
		T		Value;

		CNode(const T& Val): pNext(nullptr), pPrev(nullptr), Value(Val) {}
		~CNode() { n_assert(!pNext && !pPrev); }
	};

	CNode* pFront;
	CNode* pBack;

public:

	class CIterator
	{
	private:

		friend class CList;

		CNode* pNode;

	public:

		CIterator(): pNode(nullptr) {}
		CIterator(CNode* Node): pNode(Node) {}
		CIterator(const CIterator& Other): pNode(Other.pNode) {}

		bool				operator ==(const CIterator& Other) const { return pNode == Other.pNode; }
		bool				operator !=(const CIterator& Other) const { return pNode != Other.pNode; }
		const CIterator&	operator =(const CIterator& Other) { pNode = Other.pNode; return *this; }
		const CIterator&	operator ++() { n_assert_dbg(pNode); pNode = pNode->pNext; return *this; }
		CIterator			operator ++(int);
		const CIterator&	operator --() { n_assert_dbg(pNode); pNode = pNode->pPrev; return *this; }
		CIterator			operator --(int);
							operator bool() const { return !!pNode; }
		T*					operator ->() const { n_assert_dbg(pNode); return &pNode->Value; }
		T&					operator *() const { n_assert_dbg(pNode); return pNode->Value; }
	};

	CList(): pFront(nullptr), pBack(nullptr) {}
	CList(const CList<T>& Other): pFront(nullptr), pBack(nullptr) { AddList(Other); }
	~CList() { Clear(); }

	//???add to collections API method family T& Add...() with no const T& param, adding an empty element
	//and returning an object to fill. This will help to avoid copying!
	CIterator	Add(const T& Val) { return AddAfter(pBack, Val); }
	CIterator	AddBefore(CIterator It, const T& Val);
	CIterator	AddAfter(CIterator It, const T& Val);
	CIterator	AddFront(const T& Val) { return AddBefore(pFront, Val); }
	CIterator	AddBack(const T& Val) { return AddAfter(pBack, Val); }
	void		AddList(const CList<T>& Other);

	bool		RemoveFront(T* pOutValue = nullptr) { if (!pFront) FAIL; Remove(pFront, pOutValue); OK; }
	bool		RemoveBack(T* pOutValue = nullptr) { if (!pBack) FAIL; Remove(pBack, pOutValue); OK; }
	void		Remove(CIterator It, T* pOutValue = nullptr);
	bool		RemoveByValue(const T& Val);
	void		Clear() { while (pBack) RemoveBack(); }

	T&			Front() const { n_assert_dbg(pFront); return pFront->Value; }
	T&			Back() const { n_assert_dbg(pBack); return pBack->Value; }
	CIterator	Begin() const { return CIterator(pFront); }
	CIterator	End() const { return CIterator(nullptr); }

	CIterator	Find(const T& Val, CIterator ItStart = Begin()) const;

	bool		IsEmpty() const { return !pFront; }
	UPTR		GetCount() const { UPTR Size = 0; for (CIterator It = Begin(); It != End(); ++It) ++Size; return Size; }

	void operator =(const CList<T>& Other) { Clear(); AddList(); }
};

template<class T>
typename CList<T>::CIterator CList<T>::CIterator::operator ++(int)
{
	n_assert_dbg(pNode);
	CIterator Tmp(pNode);
	pNode = pNode->pNext;
	return Tmp;
}
//---------------------------------------------------------------------

template<class T>
typename CList<T>::CIterator CList<T>::CIterator::operator --(int)
{
	n_assert_dbg(pNode);
	CIterator Tmp(pNode);    
	pNode = pNode->pPrev;
	return Tmp;
}
//---------------------------------------------------------------------

template<class T>
inline void CList<T>::AddList(const CList<T>& Other)
{
	for (CIterator It = Other.Begin(); It != Other.End(); ++It)
		AddBack(*It);
}
//---------------------------------------------------------------------

template<class T>
typename CList<T>::CIterator CList<T>::AddAfter(CIterator It, const T& Val)
{
	CNode* pNode = n_new(CNode(Val));
	if (It.pNode)
	{
		if (It.pNode == pBack) pBack = pNode;
		if (It.pNode->pNext)
			It.pNode->pNext->pPrev = pNode;
		pNode->pNext = It.pNode->pNext;
		It.pNode->pNext = pNode;
		pNode->pPrev = It.pNode;
	}
	else
	{
		n_assert_dbg(!pFront && !pBack);
		pFront = pNode;
		pBack  = pNode;
	}
	return CIterator(pNode);
}
//---------------------------------------------------------------------

template<class T>
typename CList<T>::CIterator CList<T>::AddBefore(CIterator It, const T& Val)
{
	CNode *pNode = n_new(CNode(Val));
	if (It.pNode)
	{
		if (It.pNode == pFront) pFront = pNode;
		if (It.pNode->pPrev)
			It.pNode->pPrev->pNext = pNode;
		pNode->pPrev = It.pNode->pPrev;
		It.pNode->pPrev = pNode;
		pNode->pNext = It.pNode;
	}
	else
	{
		n_assert_dbg(!pFront && !pBack);
		pFront = pNode;
		pBack = pNode;
	}
	return CIterator(pNode);
}
//---------------------------------------------------------------------

template<class T>
void CList<T>::Remove(CIterator It, T* pOutValue)
{
	n_assert_dbg(It.pNode);
	CNode* pNode = It.pNode;
	if (pNode->pPrev) pNode->pPrev->pNext = pNode->pNext;
	if (pNode->pNext) pNode->pNext->pPrev = pNode->pPrev;
	if (pNode == pFront) pFront = pNode->pNext;
	if (pNode == pBack) pBack = pNode->pPrev;
	pNode->pNext = nullptr;
	pNode->pPrev = nullptr;
	if (pOutValue) *pOutValue = pNode->Value;
	n_delete(pNode);
}
//---------------------------------------------------------------------

template<class T>
typename CList<T>::CIterator CList<T>::Find(const T& Val, CIterator ItStart) const
{
	for (; ItStart != End(); ++ItStart)
		if (*ItStart == Val)
			return ItStart;
	return nullptr;
}
//---------------------------------------------------------------------

}

#endif