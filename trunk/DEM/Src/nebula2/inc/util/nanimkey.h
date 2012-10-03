#ifndef N_ANIMKEY_H
#define N_ANIMKEY_H
//------------------------------------------------------------------------------
/**
    @class nAnimKey
    @ingroup Util
    @brief Associate a data value with a point in time.

    (C) 2005 Radon Labs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
template<class TYPE> class nAnimKey
{
public:
    /// default constructor
    nAnimKey();
    /// construct with time and value
    nAnimKey(float t, const TYPE& v);
    /// copy constructor
    nAnimKey(const nAnimKey& rhs);
    /// set time
    void SetTime(float t);
    /// get time
    float GetTime() const;
    /// set value
    void SetValue(const TYPE& v);
    /// get value
    const TYPE& GetValue() const;
    /// set to interpolated value
    void Lerp(const TYPE& key0, const TYPE& key1, float l);
    /// only compares time
    bool operator< (const nAnimKey& right) const;
    /// only compares time
    bool operator> (const nAnimKey& right) const;

private:
    float time;
    TYPE value;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nAnimKey<TYPE>::nAnimKey() :
    time(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nAnimKey<TYPE>::nAnimKey(float t, const TYPE& v) :
    time(t),
    value(v)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
nAnimKey<TYPE>::nAnimKey(const nAnimKey& rhs) :
    time(rhs.time),
    value(rhs.value)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nAnimKey<TYPE>::SetTime(float t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
float
nAnimKey<TYPE>::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nAnimKey<TYPE>::SetValue(const TYPE& v)
{
    this->value = v;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
nAnimKey<TYPE>::GetValue() const
{
    return this->value;
}

//------------------------------------------------------------------------------
/**
     - 09-01-2005    bruce      Change to use lerp<TYPE>(...) to allow proper
                                interpolation of quaternions.
*/
template<class TYPE>
void
nAnimKey<TYPE>::Lerp(const TYPE& key0, const TYPE& key1, float l)
{
    lerp<TYPE>(this->value, key0, key1, l);
}
//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool nAnimKey<TYPE>::operator<(const nAnimKey<TYPE>& right) const
{
    return this->GetTime() < right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool nAnimKey<TYPE>::operator>(const nAnimKey<TYPE>& right) const
{
    return this->GetTime() > right.GetTime();
}

//------------------------------------------------------------------------------

#endif
