#ifndef N_THREADVARIABLE_H
#define N_THREADVARIABLE_H
//------------------------------------------------------------------------------
/**
    @class nThreadVariable
    @ingroup NebulaThreadingSupport
    @ingroup NebulaDataTypes

    @brief A thread safe variable of any type. Protects assignments with a mutex.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/nmutex.h"

//------------------------------------------------------------------------------
template<class TYPE> class nThreadVariable
{
public:
    /// default constructor
    nThreadVariable();
    /// copy constructor
    nThreadVariable(const nThreadVariable& src);
    /// assignment constructor
    nThreadVariable(TYPE val);
    /// assignment operator with ThreadVariable
    void operator=(const nThreadVariable& rhs);
    /// assignment operator with value
    void operator=(TYPE rhs);
    /// set value
    void Set(TYPE val);
    /// get content
    TYPE Get() const;

private:
    mutable nMutex mutex;
    TYPE value;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadVariable<TYPE>::Set(TYPE val)
{
    this->mutex.Lock();
    this->value = val;
    this->mutex.Unlock();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
TYPE
nThreadVariable<TYPE>::Get() const
{
    this->mutex.Lock();
    TYPE retval = this->value;
    this->mutex.Unlock();
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nThreadVariable<TYPE>::nThreadVariable()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nThreadVariable<TYPE>::nThreadVariable(const nThreadVariable& src)
{
    this->Set(src.Get());
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nThreadVariable<TYPE>::nThreadVariable(TYPE val)
{
    this->Set(val);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadVariable<TYPE>::operator=(const nThreadVariable& rhs)
{
    this->Set(rhs.Get());
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadVariable<TYPE>::operator=(TYPE rhs)
{
    this->Set(rhs);
}

//------------------------------------------------------------------------------
#endif


