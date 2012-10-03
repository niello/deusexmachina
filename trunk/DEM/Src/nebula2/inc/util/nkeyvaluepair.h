#ifndef N_KEYVALUEPAIR_H
#define N_KEYVALUEPAIR_H
//------------------------------------------------------------------------------
/**
    @class nKeyValuePair

    Key/Value pair objects are used by most associative container classes,
    like Dictionary or HashTable. This has been "backported" from
    the Nebula3 Core Layer.

    (C) 2006 Radon Labs GmbH
*/
#include "kernel/ntypes.h"

//------------------------------------------------------------------------------
template<class KEYTYPE, class VALUETYPE> class nKeyValuePair
{
public:
    /// default constructor
    nKeyValuePair();
    /// constructor with key and value
    nKeyValuePair(const KEYTYPE& k, const VALUETYPE& v);
    /// constructor with key and undefined value
    nKeyValuePair(const KEYTYPE& k);
    /// copy constructor
    nKeyValuePair(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs);
    /// assignment operator
    void operator=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs);
    /// equality operator
    bool operator==(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// greater operator
    bool operator>(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// lesser operator
    bool operator<(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// lesser-or-equal operator
    bool operator<=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const;
    /// read/write access to value
    VALUETYPE& Value();
    /// read access to key
    const KEYTYPE& Key() const;
    /// read access to key
    const VALUETYPE& Value() const;

protected:
    KEYTYPE key;
    VALUETYPE value;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nKeyValuePair<KEYTYPE, VALUETYPE>::nKeyValuePair()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nKeyValuePair<KEYTYPE, VALUETYPE>::nKeyValuePair(const KEYTYPE& k, const VALUETYPE& v) :
    key(k),
    value(v)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This strange constructor is useful for search-by-key if
    the key-value-pairs are stored in an Util::Array.
*/
template<class KEYTYPE, class VALUETYPE>
nKeyValuePair<KEYTYPE, VALUETYPE>::nKeyValuePair(const KEYTYPE& k) :
    key(k)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
nKeyValuePair<KEYTYPE, VALUETYPE>::nKeyValuePair(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) :
    key(rhs.key),
    value(rhs.value)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline void
nKeyValuePair<KEYTYPE, VALUETYPE>::operator=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs)
{
    this->key = rhs.key;
    this->value = rhs.value;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator==(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key == rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator!=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key != rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator>(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key > rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator>=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key >= rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator<(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key < rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
nKeyValuePair<KEYTYPE, VALUETYPE>::operator<=(const nKeyValuePair<KEYTYPE, VALUETYPE>& rhs) const
{
    return (this->key <= rhs.key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline VALUETYPE&
nKeyValuePair<KEYTYPE, VALUETYPE>::Value()
{
    return this->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline const KEYTYPE&
nKeyValuePair<KEYTYPE, VALUETYPE>::Key() const
{
    return this->key;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
inline const VALUETYPE&
nKeyValuePair<KEYTYPE, VALUETYPE>::Value() const
{
    return this->value;
}
//------------------------------------------------------------------------------
#endif
