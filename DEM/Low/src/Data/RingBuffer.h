#pragma once
#ifndef N_RINGBUFFER_H
#define N_RINGBUFFER_H

#include <StdDEM.h>

// A ring buffer class
// (C) 2003 RadonLabs GmbH

template<class T>
class CRingBuffer
{
private:

	T* pStart;	// Start of ring buffer array
	T* pEnd;	// Last element of ring buffer array
	T* pHead;	// Youngest element + 1
	T* pTail;	// Oldest valid element

	void Copy(const CRingBuffer<T>& Other);

public:

	CRingBuffer(): pStart(NULL), pEnd(NULL), pHead(NULL), pTail(NULL) {}
	CRingBuffer(UPTR Capacity): pStart(NULL) { Initialize(Capacity); }
	~CRingBuffer() { if (pStart) n_delete_array(pStart); }

	void Initialize(UPTR Capacity);

	T* Add();
	void DeleteTail();

	T* GetStart() const { return pStart; }
	T* GetEnd() const { return pEnd; }
	T* GetHead() const;
	T* GetTail() const { return IsEmpty() ? NULL : pTail; }
	T* GetNext(T* pElm) const;
	T* GetPrev(T* pElm) const;

	bool IsValid() const { return !!pStart; }
	bool IsEmpty() const { return pHead == pTail; }
	bool IsFull() const;

	CRingBuffer<T>& operator =(const CRingBuffer<T>& Other);
};

template<class T> void CRingBuffer<T>::Copy(const CRingBuffer<T>& Other)
{
	if (!pStart) return;
	int Capacity = Other.pEnd - Other.pStart;
	pStart = n_new_array(T, Capacity + 1);
	pEnd = pStart + Capacity;
	pTail = pStart;
	pHead = pStart;
	memcpy(Other.pStart, pStart, Capacity + 1);
}
//---------------------------------------------------------------------

template<class T> void CRingBuffer<T>::Initialize(UPTR Capacity)
{
	n_assert(!pStart);
	//_num++; // there is always 1 empty element in buffer
	pStart = n_new_array(T, Capacity + 1);
	pEnd = pStart + Capacity;
	pTail = pStart;
	pHead = pStart;
}
//---------------------------------------------------------------------

template<class T> CRingBuffer<T>& CRingBuffer<T>::operator =(const CRingBuffer<T>& Other)
{
	if (pStart) n_delete_array(pStart);
	Copy(Other);
	return *this;
}
//---------------------------------------------------------------------

template<class T> bool CRingBuffer<T>::IsFull() const
{
	T* pElm = pHead;
	if (pElm == pEnd) pElm = pStart;
	else ++pElm;
	return pElm == pTail;
}
//---------------------------------------------------------------------

template<class T> T* CRingBuffer<T>::Add()
{
	n_assert(pStart);
	n_assert(!IsFull());
	T* pElm = pHead;
	if (pHead == pEnd) pHead = pStart;
	else pHead++;
	return pElm;
}
//---------------------------------------------------------------------

template<class T> void CRingBuffer<T>::DeleteTail()
{
	n_assert(pStart && !IsEmpty());
	if (pTail == pEnd) pTail = pStart;
	else pTail++;
}
//---------------------------------------------------------------------

template<class T> inline T* CRingBuffer<T>::GetHead() const
{
	if (IsEmpty()) return NULL;
	T* pElm = pHead - 1;
	if (pElm < pStart) pElm = pEnd;
	return pElm;
}
//---------------------------------------------------------------------

template<class T> T* CRingBuffer<T>::GetNext(T* pElm) const
{
	n_assert(pElm && pStart);
	if (pElm == pEnd) pElm = pStart;
	else ++pElm;
	return (pElm == pHead) ? NULL : pElm;
}
//---------------------------------------------------------------------

// Get previous element (from pHead to pTail).
template<class T> inline T* CRingBuffer<T>::GetPrev(T* pElm) const
{
	n_assert(pElm);
	if (IsEmpty()) return NULL;
	return (pElm == pStart) ? pEnd : pElm - 1;
}
//---------------------------------------------------------------------

#endif
