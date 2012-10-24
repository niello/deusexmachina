#ifndef N_THREADVARIABLE_H
#define N_THREADVARIABLE_H

#include <kernel/nmutex.h>

// A thread safe variable of any type. Protects assignments with a Mutex.
// (C) 2003 RadonLabs GmbH

template<class T>
class CThreadVar
{
private:

	mutable nMutex	Mutex;
	T				Value;

public:

	CThreadVar() {}
	CThreadVar(const CThreadVar& Other) { Set(Other.Get()); }
	CThreadVar(T Val) { Set(Val); }

	void	Set(T Val);
	T		Get() const;

	void operator =(const CThreadVar& Other) { Set(Other.Get()); }
	void operator =(T Val) { Set(Val); }
};

template<class T> void CThreadVar<T>::Set(T Val)
{
	Mutex.Lock();
	Value = Val;
	Mutex.Unlock();
}
//---------------------------------------------------------------------

template<class T> T CThreadVar<T>::Get() const
{
	Mutex.Lock();
	T retval = Value;
	Mutex.Unlock();
	return retval;
}
//---------------------------------------------------------------------

#endif


