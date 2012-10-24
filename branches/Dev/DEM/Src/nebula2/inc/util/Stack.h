#ifndef N_STACK_H
#define N_STACK_H

#include <util/nobjectlist.h>

// A stack template class.
// (C) 2002 RadonLabs GmbH

template<class T>
class CStack
{
private:

	nObjectList<T> List;

public:

	~CStack();

	void		Push(const T& Elm) { List.AddHead(n_new(nObjectNode<T>(Elm))); }
	T			Pop();
	const T&	Top() const;
	bool		IsEmpty() const { return List.IsEmpty(); }
};

template<class T>
CStack<T>::~CStack()
{
	nObjectNode<T>* pNode;
	while (pNode = List.RemHead())
		n_delete(pNode);
}
//---------------------------------------------------------------------

template<class T>
T CStack<T>::Pop()
{
	nObjectNode<T>* pNode = List.RemHead();
	if (pNode)
	{
		T Obj = pNode->Object;
		n_delete(pNode);
		return Obj;
	}
	else return 0;
}
//---------------------------------------------------------------------

template<class T>
const T& CStack<T>::Top() const
{
	nObjectNode<T>* pNode = List.GetHead();
	n_assert(pNode);
	return pNode->Object;
}
//---------------------------------------------------------------------

#endif

